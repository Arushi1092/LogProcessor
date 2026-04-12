from fastapi import FastAPI, HTTPException, Depends
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy.orm import Session
import subprocess, json, os, time

import schemas, crud
from database import get_db, engine
import models

# Create all tables on startup
models.Base.metadata.create_all(bind=engine)

BINARY_PATH = os.getenv("BINARY_PATH", "./LogProcessor")

app = FastAPI(title="Log Processor API", version="2.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/health")
def health_check():
    return {"status": "ok", "service": "Log Processor API"}

@app.post("/process", response_model=schemas.ProcessResponse)
def process_logs(request: schemas.ProcessRequest, db: Session = Depends(get_db)):
    if not os.path.exists(BINARY_PATH):
        raise HTTPException(status_code=500, detail=f"Binary not found at {BINARY_PATH}")

    log_file = request.log_file
    if ".." in log_file or log_file.startswith("/"):
        raise HTTPException(status_code=400, detail="Invalid log file path")

    start = time.time()
    try:
        result = subprocess.run(
            [BINARY_PATH, "--json", log_file],
            capture_output=True, text=True, timeout=30,
            cwd=r"E:\DVR_practice\LogProcessor"
        )

        elapsed_ms = (time.time() - start) * 1000

        if result.returncode != 0:
            error_msg = result.stderr or "Unknown error"
            crud.save_failed_run(db, log_file, error_msg)
            raise HTTPException(status_code=500, detail=f"Binary failed: {error_msg}")

        output = result.stdout.strip()
        json_start = output.find('{')
        if json_start == -1:
            raise HTTPException(status_code=500, detail="No JSON in output")

        data = json.loads(output[json_start:])

        # ← THIS is the new Day 2 part — save to DB
        run = crud.save_run(db, log_file, data, elapsed_ms)

        return schemas.ProcessResponse(success=True, data=data, run_id=run.id)

    except subprocess.TimeoutExpired:
        raise HTTPException(status_code=504, detail="Timed out")
    except json.JSONDecodeError as e:
        raise HTTPException(status_code=500, detail=f"JSON parse error: {e}")

@app.get("/runs", response_model=list[schemas.RunSummary])
def get_runs(limit: int = 50, db: Session = Depends(get_db)):
    return crud.get_all_runs(db, limit)

@app.get("/runs/{run_id}", response_model=schemas.RunDetail)
def get_run(run_id: int, db: Session = Depends(get_db)):
    run = crud.get_run_by_id(db, run_id)
    if not run:
        raise HTTPException(status_code=404, detail="Run not found")
    return run

@app.get("/")
def root():
    return {"service": "Log Processor API", "version": "2.0.0"}
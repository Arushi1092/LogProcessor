from sqlalchemy.orm import Session
from models import ProcessingRun
from typing import List, Optional

def save_run(db: Session, log_file: str, raw_json: dict,
             processing_time_ms: float) -> ProcessingRun:
    run = ProcessingRun(
        log_file=log_file,
        raw_json=raw_json,
        total_lines=raw_json.get("total_lines"),
        error_count=raw_json.get("error_count"),
        warning_count=raw_json.get("warning_count"),
        info_count=raw_json.get("info_count"),
        processing_time_ms=processing_time_ms,
        status="success"
    )
    db.add(run)
    db.commit()
    db.refresh(run)
    return run

def save_failed_run(db: Session, log_file: str, error_msg: str) -> ProcessingRun:
    run = ProcessingRun(
        log_file=log_file,
        raw_json={},
        status="error",
        error_message=error_msg
    )
    db.add(run)
    db.commit()
    db.refresh(run)
    return run

def get_all_runs(db: Session, limit: int = 50) -> List[ProcessingRun]:
    return db.query(ProcessingRun)\
             .order_by(ProcessingRun.created_at.desc())\
             .limit(limit).all()

def get_run_by_id(db: Session, run_id: int) -> Optional[ProcessingRun]:
    return db.query(ProcessingRun)\
             .filter(ProcessingRun.id == run_id).first()
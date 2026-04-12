from pydantic import BaseModel
from datetime import datetime
from typing import Optional, Dict, Any

class ProcessRequest(BaseModel):
    log_file: str = "data/sample.log"

class ProcessResponse(BaseModel):
    success: bool
    data: Optional[Dict[str, Any]] = None
    error: Optional[str] = None
    run_id: Optional[int] = None

class RunSummary(BaseModel):
    id: int
    log_file: str
    total_lines: Optional[int]
    error_count: Optional[int]
    warning_count: Optional[int]
    status: str
    created_at: datetime

    class Config:
        from_attributes = True

class RunDetail(RunSummary):
    raw_json: Dict[str, Any]
    processing_time_ms: Optional[float]
from sqlalchemy import Column, Integer, String, Float, DateTime, JSON, Text
from sqlalchemy.sql import func
from database import Base

class ProcessingRun(Base):
    __tablename__ = "processing_runs"

    id                 = Column(Integer, primary_key=True, index=True)
    log_file           = Column(String, nullable=False)
    raw_json           = Column(JSON, nullable=False)
    total_lines        = Column(Integer)
    error_count        = Column(Integer)
    warning_count      = Column(Integer)
    info_count         = Column(Integer)
    processing_time_ms = Column(Float)
    status             = Column(String, default="success")
    error_message      = Column(Text, nullable=True)
    created_at         = Column(DateTime(timezone=True), server_default=func.now())
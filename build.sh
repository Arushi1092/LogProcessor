#!/bin/bash
set -e

echo "==> Compiling C++ binary..."
g++ -std=c++17 -pthread -o LogProcessor \
    LogProcessor/src/main.cpp \
    LogProcessor/src/parser.cpp \
    LogProcessor/logprocessor.cpp \
    LogProcessor/logreader.cpp \
    LogProcessor/worker.cpp \
    -I LogProcessor/include

echo "==> Binary compiled successfully"
chmod +x LogProcessor

echo "==> Installing Python dependencies..."
pip install -r api/requirements.txt
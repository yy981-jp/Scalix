@echo off

start "scalix" /max cmd /k cd build
start "sxtr" /max cmd /k cd tracker ^& call .venv\scripts\activate
start "sxtr: cam" /max cmd /k cd tracker ^& call .venv\scripts\activate

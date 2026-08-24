@echo off

start "scalix" /max cmd /k cd build ^& chcp 65001
start "sxtr" /max cmd /k cd tracker ^& call .venv\scripts\activate
start "sxtr: cam" /max cmd /k cd tracker ^& call .venv\scripts\activate

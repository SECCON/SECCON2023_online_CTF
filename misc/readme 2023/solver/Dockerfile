FROM python:3.11-alpine

WORKDIR /app
ADD . /app

RUN apk add gcc musl-dev
RUN pip install ptrlib

CMD ["python3", "/app/solve.py"]

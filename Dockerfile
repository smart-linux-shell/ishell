FROM python:3.11.4-slim AS base

# Do not write .pyc files
ENV PYTHONDONTWRITEBYTECODE=1

WORKDIR /app

# Create a non-privileged user
ARG UID=10001
RUN adduser \
	--disabled-password \
	--gecos "" \
	--home "/nonexistent" \
	--shell "/sbin/nologin" \
	--no-create-home \
	--uid "${UID}" \
	appuser

WORKDIR "/app"

COPY . .

RUN pip install -r requirements.txt

RUN chown -R appuser:appuser /app

USER appuser

CMD streamlit run app.py

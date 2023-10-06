FROM sagemath/sagemath:9.6

USER root
COPY flag.py .
COPY problem.sage .
RUN sage --pip install pycryptodome
RUN apt-get update && apt-get install socat -y

CMD ["socat", "TCP-L:8080,fork,reuseaddr", "EXEC:'sage problem.sage'"]
# ENTRYPOINT ["/bin/sh", "-c", "while :; do sleep 10; done"]
FROM sagemath/sagemath:9.6

USER root
COPY get.py .
COPY solver.sage .
COPY solver.bash .

RUN sage --pip install pycryptodome
CMD ["bash", "solver.bash"]

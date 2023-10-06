FROM python:3

ENV DEBIAN_FRONTEND=noninteractive

MAINTAINER ShiftCrops <simiyu@shift-crops.net>

WORKDIR /app

ADD ./ /app

RUN pip3 install -r requirements.txt -c requirements.lock && \
	mkdir /opt/sc_expwn

ADD https://raw.githubusercontent.com/shift-crops/sc_expwn/master/sc_expwn.py /opt/sc_expwn/

ENV PYTHONPATH "${PYTHONPATH}:/opt/sc_expwn"
ENV TERM "linux"

CMD ["python3", "exploit.py"]

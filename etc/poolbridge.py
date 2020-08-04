#!/usr/bin/env python3

# From https://github.com/Nilhcem/home-monitoring-grafana/blob/master/02-bridge/main.py

"""A MQTT to InfluxDB Bridge

This script receives MQTT data and saves those to InfluxDB.

"""

import re
from typing import NamedTuple

import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

INFLUXDB_ADDRESS = 'localhost'
INFLUXDB_USER = 'pool'
INFLUXDB_PASSWORD = '<secret>'
INFLUXDB_DATABASE = 'pool_db'

MQTT_ADDRESS = 'localhost'
MQTT_TOPICS = ['pool/main/+', 'pool/exchanger/+']
MQTT_CLIENT_ID = 'MQTTPoolBridge'

influxdb_client = InfluxDBClient(
    INFLUXDB_ADDRESS, 8086,
    INFLUXDB_USER, INFLUXDB_PASSWORD, INFLUXDB_DATABASE
)


class TemperatureData(NamedTuple):
    measurement: str
    tag: str
    value: float


def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('MQTT connected with result code', rc)
    for topic in MQTT_TOPICS:
        print('MQTT subscribe to', topic)
        client.subscribe(topic)


# Topics are 'pool/{main,exchanger}/{temperature,setpoint}'
MQTT_REGEX = re.compile('pool/(?P<component>[^/]+)/(?P<subtype>[^/]+)')

def on_message(client, userdata, msg):
    """The callback for when a PUBLISH message is received from the server."""
    payload = msg.payload.decode('utf-8')
    print('MQTT', msg.topic, '->', payload)
    m = MQTT_REGEX.match(msg.topic)
    #print("DBG:", m)
    if m:
        data = TemperatureData(
            m['component'],
            m['subtype'],
            float(payload)
        )
        _send_sensor_data_to_influxdb(data)


def _send_sensor_data_to_influxdb(data):
    json_body = [
        {
            'measurement': data.measurement,
            'tags': {
                'tag': data.tag,
            },
            'fields': {
                'value': data.value
            }
        }
    ]
    #print("DBG:", json_body)
    influxdb_client.write_points(json_body)


def main():
    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    #mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(MQTT_ADDRESS, 1883)
    mqtt_client.loop_forever()


if __name__ == '__main__':
    print('MQTT to InfluxDB bridge')
    main()

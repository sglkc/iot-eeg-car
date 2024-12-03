import json

from flask import Flask
from flask_mqtt import Mqtt
from flask_socketio import SocketIO

app = Flask(__name__)
socketio = SocketIO(app)

app.config['MQTT_BROKER_URL'] = 'mosquitto'
app.config['MQTT_BROKER_PORT'] = 1883  # default port for non-tls connection
app.config['MQTT_KEEPALIVE'] = 5  # set the time interval for sending a ping to the broker to 5 seconds
app.config['MQTT_TLS_ENABLED'] = False

mqtt = Mqtt(app)


@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    data = dict(
        topic=message.topic,
        payload=message.payload.decode()
    )
    print(data)  # not printed in docker logs


@socketio.on('publish')
def handle_publish(json_str):
    data = json.loads(json_str)
    mqtt.publish(data['topic'], data['message'])


@socketio.on('subscribe')
def handle_subscribe(json_str):
    data = json.loads(json_str)
    mqtt.subscribe(data['topic'])


@app.route('/webhook', methods=['POST'])
def webhook():
    message = "A new entry was created!"
    print(message)  # does not show up in the docker logs
    result = mqtt.publish('DIRECTUS', bytes(message, 'utf-8'))
    print(result)
    return "Erfolg", 200


@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    mqtt.subscribe('DIRECTUS')


@mqtt.on_log()
def handle_logging(client, userdata, level, buf):
    print('LOG: {}'.format(buf))


socketio.run(app, host='app', port=8080, use_reloader=False, debug=True, allow_unsafe_werkzeug=True)


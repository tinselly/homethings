
from flask import Flask, jsonify, request
from flask_cors import CORS, cross_origin

import paho.mqtt.client as mqtt

import json

from dataclasses import dataclass, field

app = Flask(__name__)
cors = CORS(app)

app.config['CORS_HEADERS'] = 'Content-Type'

led_things: dict = {}
client = mqtt.Client()

from threading import Timer


def debounce(wait):
    """ Decorator that will postpone a functions
        execution until after wait seconds
        have elapsed since the last time it was invoked. """
    def decorator(fn):
        def debounced(*args, **kwargs):
            def call_it():
                fn(*args, **kwargs)
            try:
                debounced.t.cancel()
            except(AttributeError):
                pass
            debounced.t = Timer(wait, call_it)
            debounced.t.start()
        return debounced
    return decorator

@dataclass
class LedThing():
    id: str = ""
    colors: list[str] = field(default_factory=list)
    enabled: bool = False
    intensity: int = 50
    animation: int = 0


@app.get("/things/led/list")
def things_led_list():
    return jsonify(list(led_things.values())), 200

@debounce(0.250)
def send_led_cmd(thing_id:str,cmd:dict):
    client.publish(f"homethings/led/{thing_id}/cmd", cmd, qos=1, retain=True)

@app.post("/things/led/<thing_id>")
def things_led_set(thing_id):
    print(thing_id, request.json)
    
    colors = request.json["colors"]
    colors = map(lambda color : color.replace("#", ""), colors)
    # colors = map(lambda color : int(color, 0), colors)
    colors = list(colors)
    print(colors)
    send_led_cmd(thing_id, jsonify({
        "colors": colors,
        "intensity": request.json["intensity"],
        "enabled": request.json["enabled"],
        "animation": request.json["animation"]
    }).get_data(as_text=True))
    
    return "OK", 200


def mqtt_message(client, userdata, msg):
    topic: list[str] = msg.topic.split("/")
    if len(topic) < 3 or topic[0] != "homethings" or topic[1] != "led":
        return

    payload = msg.payload.decode("UTF-8")
    thing_id = topic[2]

    print(f"Message ({topic}) {thing_id}={payload}")

    if len(topic) == 3:
        if "Online" == payload and thing_id not in led_things:
            led_things[thing_id] = LedThing(id=thing_id)
            print("LedThing %s - Online", thing_id)
        elif "Offline" == str(msg.payload):
            del led_things[thing_id]
            print("LedThing %s - Offline", thing_id)

        return

    if len(topic) == 4 and topic[3] == "cmd":
        data = json.loads(payload)

        led_things[thing_id].colors = list(
            map(lambda color: "#"+color.replace("0x", ""), data["colors"]))
        led_things[thing_id].intensity = data["intensity"]
        led_things[thing_id].enabled = data["enabled"]
        led_things[thing_id].animation = data["animation"]


def mqtt_connected(client, userdata, flags, rc):
    print("MQTT connected")

    client.subscribe("homethings/led/#")


@app.before_first_request
def startup():
    print("Start MQTT Client")

    client.on_connect = mqtt_connected
    client.on_message = mqtt_message

    client.connect(host="192.168.1.123",
                   port=1883,
                   keepalive=60,
                   )

    client.tls_set("./homethings-ca-cert.crt",
                   "./homethings-client-cert.crt",
                   "./homethings-client-key.pem")

    client.tls_insecure_set(True)
    client.loop_start()


if __name__ == "__main__":
    app.run()

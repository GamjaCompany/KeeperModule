# 1. Flask로 웹캠 스트리밍 
# 2. Flask동작 하면서 MQTT서버 연결
# 3. Flask서버내 특정 이미지 파일 요청가능

import uuid
from flask import Flask, send_file, render_template, Response
import numpy as np
import cv2
import time
import os
import paho.mqtt.client as paho
from paho.mqtt.properties import Properties
from paho.mqtt.packettypes import PacketTypes

def StringToBytes(val):
    return [ord(c) for c in val]

def on_connect(client, userdata, flags, rc, properties=None):
    print("CONNACK received with code %s." % rc)
    cur_time = time.strftime('%H:%M:%S')
    conn_respone = cur_time + " --> 파이썬 서버 Online!"
    print(conn_respone)

def on_disconnect(client, userdata, rc, properties=None):
    print("DISCONN received with code %s." % rc)
    cur_time = time.strftime('%H:%M:%S')
    conn_respone = cur_time + " --> 파이썬 서버 OFF"
    print(conn_respone)

def on_subscribe(client, userdata, mid, granted_qos, properties=None):
    print("Subscribed: " + str(mid) + " " + str(granted_qos[0]))

def on_message(client, userdata, msg):
    payload = msg.payload.decode('utf-8')  # MQTT 수신 msg는 bytes형이므로 str형으로 변환
    print("%s : %s" % (msg.topic, payload))

cap = cv2.VideoCapture(0)
app = Flask(__name__)

def generate_frames():
    while True:
        success, frame = cap.read()  # 프레임 읽기
        if not success:
            break
        else:
            ret, buffer = cv2.imencode('.jpg', frame)
            frame = buffer.tobytes()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n')  # 프레임 스트리밍

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/video')
def getCamera():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/video/<int:image_id>', methods=['GET'])
def serve_image(image_id):
    filename = f"{image_id}.jpg"
    filepath = os.path.join("./", filename)

    if os.path.exists(filepath):
        return send_file(filepath, mimetype='image/jpeg')

# 고유한 client_id 생성
client_id = f"client-{uuid.uuid4()}"
client = paho.Client(client_id=client_id, protocol=paho.MQTTv5)

client.tls_set(tls_version=paho.ssl.PROTOCOL_TLS)  # TLS 보안 적용

client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_subscribe = on_subscribe
client.on_message = on_message

connect_props = Properties(PacketTypes.CONNECT)
connect_props.SessionExpiryInterval = 0  # 세션 유지 없이 새 세션 시작

def run_mqtt_client():
    client.username_pw_set('접근ID', '접근PW')  # 아이디와 비밀번호
    client.connect('브로커서버url', 8883, keepalive=60, properties=connect_props)
    client.subscribe('GET', qos=0)
    client.subscribe('Response', qos=0)
    client.subscribe('Control', qos=0)
    client.loop_start()

if __name__ == '__main__':
    try:
        run_mqtt_client()
        app.run('0.0.0.0', port=5000, debug=True)
    except KeyboardInterrupt:
        client.loop_stop()  # MQTT 루프 종료
        client.disconnect()  # 안전하게 연결 종료

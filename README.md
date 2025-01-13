# KeeperModule
* ## 말뚝 부 코드
  * ⚙️ <span style="color:yellow; font-weight: bold;">Module_Controller</span> : 모터 및 스피커, LED 등 액추에이터 제어
  * 👁️ <span style="color:yellow; font-weight: bold;">Module_Eye</span> : 중앙 AI 컨트롤러로 영상 전송

* ## 말뚝 부 코드
  * 🤖 <span style="color:yellow; font-weight: bold;">AI_Controller</span> : 중앙 AI 판단 및 데이터 관리

# Notice
* 사용 HW
  * AI-thinker ESP32 CAM -> 영상 전송(`./Module_Eye`)
  * ESP32 devkit v1 -> 영상 전송(`./Module_Controller`)
* 사용 기술
  * MQTT : HW통신 프로토콜(HTTP통신 대체)
  * Yolo : 객체인식(멧돼지 및 고라니 판단)
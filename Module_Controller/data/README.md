# [Data] 폴더 설명
- 해당 폴더는 `LittleFS`저장소에 업로드할 데이터를 저장합니다.
- 대표적으로 `env.txt`가 있으며, 해당 파일에는 다음과 같은 내용을 작성해야 합니다
  - wifi 이름
  - wifi 비밀번호
  - mqtt broker 주소
  - broker 접근 ID
  - broker 접근 password

---
# `env.txt` 문법
- `JSON`형식으로 작성해야합니다.
- 작성 예시
```
{
  "wifi": {
    "<SSID1>": ["<PASSWORD1>", 0],
    "<SSID2>": ["<PASSWORD2>", 0],
    "<SSID3>": ["<PASSWORD3>", 0]
  },
  "mqtt": {
    "broker_address": "<BROKER_ADDRESS>",
    "port": <BROKER_PORT>,
    "user_id": "<USER_ID>",
    "user_password": "<USER_PASSWORD>"
  }
}
```


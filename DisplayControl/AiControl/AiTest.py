
import json
import time
import websocket
from openai import OpenAI

# =====================================================
# CONFIG
# =====================================================

JAN_API = "http://127.0.0.1:1337/v1"
MODEL = "Jan-v3.5-4B-Q4_K_XL"

ESP32_WS_URL = "ws://172.20.10.2:8765"

# =====================================================
# CONNECT TO JAN
# =====================================================

client = OpenAI(
    base_url=JAN_API,
    api_key="jan"
)

# =====================================================
# CONNECT TO ESP32
# =====================================================

print("Connecting to ESP32...")

ws = websocket.create_connection(
    ESP32_WS_URL,
    timeout=5
)

print("Connected.")

# =====================================================
# EYE COMMANDS
# =====================================================

def send_eye_command(cmd, value=None):

    payload = {"cmd": cmd}

    if value is not None:
        payload["value"] = value

    ws.send(json.dumps(payload))

    print("[EYES]", payload)


def set_state(state):

    if state == "DEBUG":
        return

    send_eye_command(
        "state",
        state
    )


# =====================================================
# INITIAL STATE
# =====================================================

set_state("IDLE")

# =====================================================
# REACTION EXECUTION
# =====================================================


def execute_reaction(reaction):

    print("\n========== AI REACTION ==========")
    print(json.dumps(reaction, indent=2))
    print("=================================\n")

    commands_sent = []

    def send_and_log(cmd, value=None):

        commands_sent.append({
            "cmd": cmd,
            "value": value
        })

        send_eye_command(
            cmd,
            value
        )

    state = reaction.get("state")

    if state == "DEBUG":
        print("DEBUG state requested. Ignored.")
        state = None

    active_mode = state == "ACTIVE"

    if state:
        send_and_log(
            "state",
            state
        )

    mood = reaction.get("mood")

    if mood:
        send_and_log(
            "mood",
            mood
        )

    pupil = reaction.get("pupil")

    if pupil is not None:

        pupil = max(
            0.0,
            min(
                1.0,
                float(pupil)
            )
        )

        send_and_log(
            "pupilR",
            {"r": pupil}
        )

    look = reaction.get("look")

    if look:

        x = max(
            0.0,
            min(
                1.0,
                float(look["x"])
            )
        )

        y = max(
            0.0,
            min(
                1.0,
                float(look["y"])
            )
        )

        send_and_log(
            "look",
            {
                "x": x,
                "y": y
            }
        )

    glance = reaction.get("glance")

    if glance:

        x = max(
            -1.0,
            min(
                1.0,
                float(glance["x"])
            )
        )

        y = max(
            -1.0,
            min(
                1.0,
                float(glance["y"])
            )
        )

        send_and_log(
            "glance",
            {
                "x": x,
                "y": y
            }
        )

    if reaction.get("blink", False):

        send_and_log("blink")

    if active_mode:

        time.sleep(1.5)

        send_and_log(
            "state",
            "CONV"
        )

    print("\n========== COMMANDS SENT ==========")

    for i, cmd in enumerate(commands_sent, start=1):
        print(
            f"{i}. "
            f"{json.dumps(cmd)}"
        )

    print("===================================\n")




# =====================================================
# SYSTEM PROMPT
# =====================================================

SYSTEM_PROMPT = """
You are a friendly robot.

Return ONLY valid JSON.

Format:

{
  "reply": "text for the user",

  "reaction":
  {
    "state":"CONV",
    "mood":"HAPPY",

    "look":
    {
      "x":0.5,
      "y":0.5
    },

    "glance":
    {
      "x":0.2,
      "y":-0.3
    },

    "pupil":0.4,

    "blink":false
  }
}

Allowed moods:

NEUTRAL
HAPPY
SAD
SURPRISED
CONFUSED

Allowed states:

IDLE
CONV
ACTIVE

NEVER USE DEBUG.

State rules:

IDLE:
- Used only when nobody is interacting.
- Eyes already perform autonomous idle animation.
- Usually do not choose this yourself.

CONV:
- Default state during conversation.
- Use for almost all replies.
- Eyes already blink and glance naturally.

ACTIVE:
- Use only when performing a deliberate action.
- Examples:
  - dramatic surprise
  - intentional look direction
  - exaggerated reaction
  - intentional blink

Examples:

User:
Hello

Output:

{
  "reply":"Hello! Nice to meet you.",

  "reaction":
  {
    "state":"CONV",
    "mood":"HAPPY",
    "blink":true
  }
}

User:
Wow!

Output:

{
  "reply":"That is exciting!",

  "reaction":
  {
    "state":"ACTIVE",
    "mood":"SURPRISED",
    "blink":true,
    "pupil":0.8
  }
}

Keep reactions simple.
Use ACTIVE sparingly.
Return JSON only.
"""

# =====================================================
# CHAT LOOP
# =====================================================

messages = [
    {
        "role": "system",
        "content": SYSTEM_PROMPT
    }
]

print("Robot ready.")
print("Type quit to exit.")

while True:

    user_text = input("\nYou: ")

    if user_text.lower() == "quit":
        break

    # Someone is talking to the robot
    set_state("CONV")

    messages.append(
        {
            "role": "user",
            "content": user_text
        }
    )

    response = client.chat.completions.create(
        model=MODEL,
        messages=messages,
        temperature=0.8
    )

    raw = response.choices[0].message.content

    try:

        data = json.loads(raw)

        reply = data.get(
            "reply",
            "..."
        )

        reaction = data.get(
            "reaction",
            {}
        )

        execute_reaction(reaction)

        print(
            "\nRobot:",
            reply
        )

        messages.append(
            {
                "role": "assistant",
                "content": raw
            }
        )

    except Exception as e:

        print("\nInvalid AI response")
        print(raw)
        print(e)

# =====================================================
# SHUTDOWN
# =====================================================

try:
    set_state("IDLE")
finally:
    ws.close()

print("Disconnected.")


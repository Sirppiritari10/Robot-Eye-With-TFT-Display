import json
import serial
import serial.tools.list_ports
import threading
import websocket


TARGET_SERIAL = "48:27:E2:E7:D2:C4"# Check your target serial, use ai or smth to figure out what is right
BAUD_RATE = 9600 # make sure this matches the baud rate in your arduino code

ESP32_WS_URL = "ws://172.20.10.2:8765"# use ai or smth to figure out what is right, (that's how I did it)
# -------------------------------
# CONFIG (EASY TO EXTEND)
# -------------------------------

COMMANDS = {
    "state": {
        "prompt": "State",
        "fields": ["value"],
        "options": ["IDLE", "ACTIVE", "DEBUG", "CONV"]
    },

    "look": {
        "prompt": "Look (x,y)",
        "fields": ["x", "y"],
        "float_range": (0.0, 1.0)
    },

    "mood": {
        "prompt": "Mood",
        "fields": ["value"],
        "options": ["NEUTRAL", "HAPPY", "SAD", "SURPRISED", "CONFUSED"]
    },

    "pupilR":{
        "prompt": "Pupil radius",
        "fields": ["r"],
        "float_range": (0.0, 1.0)

    },

    "blink": {
        "prompt": "Blink"
    },

    "glance": 
    {
    "prompt": "Glance",
    "optional_fields": ["x", "y"],
    "float_range": (-1.0, 1.0)
    }

}

class SerialTransport:
    def __init__(self, ser):
        self.ser = ser

    def send(self, payload):
        json_string = json.dumps(payload)

        self.ser.write((json_string + "\n").encode("utf-8"))
        self.ser.flush()

        print(f"\nSent (Serial): {json_string}")

    def close(self):
        self.ser.close()


class WebSocketTransport:
    def __init__(self, ws):
        self.ws = ws

    def send(self, payload):
        json_string = json.dumps(payload)

        self.ws.send(json_string)

        print(f"\nSent (WiFi): {json_string}")

    def close(self):
        self.ws.close()





# -------------------------------
# SERIAL PORT FINDER
# -------------------------------
def find_port_by_serial(serial_number):
    for port in serial.tools.list_ports.comports():
        if port.serial_number == serial_number:
            return port.device
    return None


# -------------------------------
# SERIAL READER
# -------------------------------
def serial_reader(ser):
    while True:
        try:
            line = ser.readline().decode(
                "utf-8",
                errors="ignore"
            ).strip()

            if line:
                print(f"\nESP32: {line}")

        except Exception:
            break



#
# Wi-Fi reader
#

def websocket_reader(ws):
    while True:
        try:
            msg = ws.recv()

            if msg:
                print(f"\nESP32: {msg}")

        except Exception:
            break

# -------------------------------
# SEND JSON
# -------------------------------
def send_command(transport, payload):
    try:
        transport.send(payload)

    except Exception as e:
        print(f"Send failed: {e}")

# -------------------------------
# GENERIC COMMAND HANDLER
# -------------------------------
def handle_command(transport, cmd_name, config):
    payload = {"cmd": cmd_name}

    values = {}

    # Enum-style command
    if "options" in config:
        print(f"{config['prompt']} options: {', '.join(config['options'])}")
        user_input = input("> ").strip()

        if user_input not in config["options"]:
            print("Invalid option")
            return

        payload["value"] = user_input
        send_command(transport, payload)
        return

    # Required fields
    for field in config.get("fields", []):
        raw = input(f"{field}: ").strip()

        if "float_range" in config:
            try:
                v = float(raw.replace(",", "."))
                min_v, max_v = config["float_range"]
                v = max(min_v, min(max_v, v))
            except ValueError:
                print("Invalid float input")
                return

            values[field] = v
        else:
            values[field] = raw

    # Optional fields
    for field in config.get("optional_fields", []):
        raw = input(f"{field} (blank to skip): ").strip()

        if raw == "":
            continue

        if "float_range" in config:
            try:
                v = float(raw.replace(",", "."))
                min_v, max_v = config["float_range"]
                v = max(min_v, min(max_v, v))
            except ValueError:
                print("Invalid float input")
                return

            values[field] = v
        else:
            values[field] = raw

    # Only include "value" if something was supplied
    if values:
        payload["value"] = values

    send_command(transport, payload)


# -------------------------------
# MENU BUILDER (FROM CONFIG)
# -------------------------------
def menu(transport):
    cmd_keys = list(COMMANDS.keys())

    while True:
        print("\n--- ESP32 Eye Controller ---")

        for i, key in enumerate(cmd_keys, 1):
            print(f"{i}. {key}")

        print(f"{len(cmd_keys)+1}. Quit")

        choice = input("> ").strip()

        if choice == str(len(cmd_keys)+1):
            break

        if (
            not choice.isdigit()
            or int(choice) not in range(1, len(cmd_keys)+1)
        ):
            print("Invalid option")
            continue

        cmd_name = cmd_keys[int(choice)-1]

        handle_command(
            transport,
            cmd_name,
            COMMANDS[cmd_name]
        )

# -------------------------------
# MAIN
# -------------------------------
transport = None

port_name = find_port_by_serial(TARGET_SERIAL)

if port_name is not None:

    print(f"Connecting via Serial: {port_name}")

    ser = serial.Serial(
        port=port_name,
        baudrate=BAUD_RATE,
        timeout=1
    )

    transport = SerialTransport(ser)

    threading.Thread(
        target=serial_reader,
        args=(ser,),
        daemon=True
    ).start()

else:

    print("Serial device not found")
    print("Attempting WiFi connection...")

    try:
        ws = websocket.create_connection(
            ESP32_WS_URL,
            timeout=5
        )

        transport = WebSocketTransport(ws)

        threading.Thread(
            target=websocket_reader,
            args=(ws,),
            daemon=True
        ).start()

        print("Connected via WiFi")

    except Exception as e:
        print(f"WiFi connection failed: {e}")
        exit(1)


try:
    menu(transport)

finally:
    transport.close()
    print("Disconnected")

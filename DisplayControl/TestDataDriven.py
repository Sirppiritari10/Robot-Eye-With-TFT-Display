import json
import serial
import serial.tools.list_ports
import threading

TARGET_SERIAL = "48:27:E2:E7:D1:C4"
BAUD_RATE = 9600


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
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if line:
                print(f"\nESP32: {line}")
        except Exception as e:
            print(f"Read error: {e}")
            break


# -------------------------------
# SEND JSON
# -------------------------------
def send_command(ser, payload):
    try:
        json_string = json.dumps(payload)
        ser.write((json_string + "\n").encode("utf-8"))
        ser.flush()

        print(f"\nSent: {json_string}")

    except Exception as e:
        print(f"Send failed: {e}")


# -------------------------------
# GENERIC COMMAND HANDLER
# -------------------------------
def handle_command(ser, cmd_name, config):
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
        send_command(ser, payload)
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

    send_command(ser, payload)


# -------------------------------
# MENU BUILDER (FROM CONFIG)
# -------------------------------
def menu(ser):
    cmd_keys = list(COMMANDS.keys())

    while True:
        print("\n--- ESP32 Eye Controller ---")

        for i, key in enumerate(cmd_keys, 1):
            print(f"{i}. {key}")

        print(f"{len(cmd_keys) + 1}. Quit")

        choice = input("> ").strip()

        if choice == str(len(cmd_keys) + 1):
            break

        if not choice.isdigit() or int(choice) not in range(1, len(cmd_keys) + 1):
            print("Invalid option")
            continue

        cmd_name = cmd_keys[int(choice) - 1]
        handle_command(ser, cmd_name, COMMANDS[cmd_name])


# -------------------------------
# MAIN
# -------------------------------
port_name = find_port_by_serial(TARGET_SERIAL)

if port_name is None:
    print("ESP32 not found")
    exit(1)

print(f"Connecting to {port_name}")

ser = serial.Serial(
    port=port_name,
    baudrate=BAUD_RATE,
    timeout=1
)

print("Connected")

threading.Thread(
    target=serial_reader,
    args=(ser,),
    daemon=True
).start()

menu(ser)

ser.close()
print("Disconnected")
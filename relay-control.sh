#!/bin/bash

# Guardian 1PM Relay Control
# Interactive MQTT relay control script

BROKER="34.238.60.126"
PORT="1883"
USER_ID="test-user-uuid"
DEVICE_ID="test-device-uuid"
TOPIC="/toDevice/$USER_ID/$DEVICE_ID"

echo "========================================="
echo "   Guardian 1PM Relay Control"
echo "========================================="
echo "Broker: $BROKER:$PORT"
echo "Topic: $TOPIC"
echo "========================================="
echo ""

function send_command() {
    local power_state=$1
    local message="{\"power\":$power_state}"

    echo "Sending: $message"
    mosquitto_pub -h "$BROKER" -p "$PORT" -t "$TOPIC" -m "$message"

    if [ $? -eq 0 ]; then
        echo "✓ Command sent successfully"
    else
        echo "✗ Failed to send command"
    fi
}

function turn_on() {
    echo ""
    echo "🔌 Turning relay ON..."
    send_command "true"
}

function turn_off() {
    echo ""
    echo "⏸  Turning relay OFF..."
    send_command "false"
}

function restart_device() {
    echo ""
    echo "🔄 Restarting device..."
    local message='{"command":"restart"}'
    mosquitto_pub -h "$BROKER" -p "$PORT" -t "$TOPIC" -m "$message"
    echo "✓ Restart command sent"
}

function listen_status() {
    echo ""
    echo "📡 Listening for relay status (press Ctrl+C to stop)..."
    mosquitto_sub -h "$BROKER" -p "$PORT" -t "/toDaemon/$USER_ID/$DEVICE_ID" -v
}

# Main menu loop
while true; do
    echo ""
    echo "Select an option:"
    echo "  1) Turn relay ON"
    echo "  2) Turn relay OFF"
    echo "  3) Restart device"
    echo "  4) Listen to relay status"
    echo "  5) Exit"
    echo ""
    read -p "Enter choice [1-5]: " choice

    case $choice in
        1)
            turn_on
            ;;
        2)
            turn_off
            ;;
        3)
            restart_device
            ;;
        4)
            listen_status
            ;;
        5)
            echo ""
            echo "Goodbye!"
            exit 0
            ;;
        *)
            echo "Invalid choice. Please enter 1-5."
            ;;
    esac
done

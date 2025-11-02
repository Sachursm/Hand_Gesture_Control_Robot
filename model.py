import cv2
import mediapipe as mp
import socket

ESP_IP = "192.168.1.123"   # <- replace with your ESP8266 IP
ESP_PORT = 4210

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((ESP_IP, ESP_PORT))
sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

def fingers_up(lm):
    tips = [4, 8, 12, 16, 20]
    f = []
    # Thumb: compare x (works for right hand mirrored view)
    f.append(1 if lm.landmark[tips[0]].x < lm.landmark[tips[0]-1].x else 0)
    # Other fingers: compare y
    for tip in tips[1:]:
        f.append(1 if lm.landmark[tip].y < lm.landmark[tip-2].y else 0)
    return f

cap = cv2.VideoCapture(0)
last_cmd = None
cmd_name = "STOP"

# Command name mapping
cmd_names = {
    'S': 'STOP',
    'F': 'FORWARD',
    'B': 'BACKWARD',
    'R': 'RIGHT',
    'L': 'LEFT'
}

with mp_hands.Hands(max_num_hands=1, min_detection_confidence=0.7, min_tracking_confidence=0.5) as hands:
    while True:
        ok, frame = cap.read()
        if not ok:
            break
        
        frame = cv2.flip(frame, 1)
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        res = hands.process(rgb_frame)
        
        cmd = 'S'
        
        if res.multi_hand_landmarks:
            for hand in res.multi_hand_landmarks:
                mp_draw.draw_landmarks(frame, hand, mp_hands.HAND_CONNECTIONS)
                f = fingers_up(hand)
                
                if f == [1, 1, 1, 1, 1]:
                    cmd = 'S'  # Stop (all fingers up)
                elif f == [0, 1, 0, 0, 0]:
                    cmd = 'F'  # Forward (index finger up)
                elif f == [0, 0, 0, 0, 1]:
                    cmd = 'B'  # Backward (pinky up)
                elif f == [0, 1, 1, 0, 0]:
                    cmd = 'R'  # Right (index + middle up)
                elif f == [1, 0, 0, 0, 0]:
                    cmd = 'L'  # Left (thumb up)
                else:
                    cmd = 'S'  # Default to stop
        
        cmd_name = cmd_names[cmd]
        
        # Send only on change to reduce traffic
        if cmd != last_cmd:
            try:
                sock.send(cmd.encode())
                print(f"Sent: {cmd_name}")
            except Exception as e:
                print("Socket error:", e)
                break
            last_cmd = cmd
        
        # Display command box with background
        text = f'{cmd_name}'
        font = cv2.FONT_HERSHEY_SIMPLEX
        font_scale = 1.2
        thickness = 3
        
        # Get text size for background box
        (text_width, text_height), baseline = cv2.getTextSize(text, font, font_scale, thickness)
        
        # Draw filled rectangle as background
        box_coords = ((10, 10), (text_width + 30, text_height + 40))
        cv2.rectangle(frame, box_coords[0], box_coords[1], (0, 0, 0), -1)
        cv2.rectangle(frame, box_coords[0], box_coords[1], (0, 255, 0), 2)
        
        # Put text on top of rectangle
        cv2.putText(frame, text, (20, text_height + 25),
                    font, font_scale, (0, 255, 0), thickness)
        
        # Display gesture hint
        hint = "Gestures: All fingers=STOP | Index=FORWARD | Pinky=BACKWARD | Index+Middle=RIGHT | Thumb=LEFT"
        cv2.putText(frame, hint, (10, frame.shape[0] - 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
        
        cv2.imshow("Gesture Control", frame)
        
        if cv2.waitKey(1) & 0xFF == 27:  # ESC key to exit
            break

cap.release()
sock.close()
cv2.destroyAllWindows()
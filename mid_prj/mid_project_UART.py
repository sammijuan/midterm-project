
import serial
import time

# ==============================
# UART 設定
# ==============================

PORT = "COM3"
BAUDRATE = 115200

# ==============================
# 連接 STM32
# ==============================

try:
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=1
    )

    print("UART 已連線")
    print("COM Port:", PORT)
    print("Baud Rate:", BAUDRATE)

except serial.SerialException as e:
    print("UART 連線失敗")
    print(e)
    raise SystemExit

# 等待連線穩定
time.sleep(1)

print("-------------------------")
print("STM32 摩斯碼播放器")
print("請輸入英文單字")
print("輸入 exit 可結束程式")
print("-------------------------")

# ==============================
# 主程式
# ==============================

try:
    while True:

        text = input("請輸入英文：").strip()

        if text.lower() == "exit":
            break

        if not text:
            print("請輸入文字")
            continue

        if len(text) > 31:
            print("最多只能輸入 31 個字元")
            continue

        if not text.isascii() or not text.isalpha():
            print("請只輸入英文字母 A-Z")
            continue

        # 加入換行字元
        message = text + "\n"

        # 傳送給 STM32
        ser.write(message.encode("utf-8"))
        ser.flush()

        print("已送出：", text)

        # 等待 STM32 回傳
        while True:

            response = ser.readline().decode(
                "utf-8",
                errors="replace"
            ).strip()

            if response:
                print("STM32:", response)

                # 收到 DONE 表示播放完成
                if response == "DONE":
                    break

except KeyboardInterrupt:
    print("\n使用者中斷程式")

finally:
    ser.close()
    print("UART 已關閉")

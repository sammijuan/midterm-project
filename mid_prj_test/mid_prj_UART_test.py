import tkinter as tk
from tkinter import messagebox
import serial


# =========================
# UART 設定
# =========================
PORT = "COM3"       # ← 如果 COM 號改變，只改這裡
BAUDRATE = 115200


# =========================
# 摩斯碼表
# =========================
MORSE_TABLE = {
    "A": ".-",
    "B": "-...",
    "C": "-.-.",
    "D": "-..",
    "E": ".",
    "F": "..-.",
    "G": "--.",
    "H": "....",
    "I": "..",
    "J": ".---",
    "K": "-.-",
    "L": ".-..",
    "M": "--",
    "N": "-.",
    "O": "---",
    "P": ".--.",
    "Q": "--.-",
    "R": ".-.",
    "S": "...",
    "T": "-",
    "U": "..-",
    "V": "...-",
    "W": ".--",
    "X": "-..-",
    "Y": "-.--",
    "Z": "--.."
}


# =========================
# UART 連線
# =========================
try:
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        timeout=1
    )

    uart_connected = True

except serial.SerialException:
    ser = None
    uart_connected = False


# =========================
# 英文轉摩斯碼
# =========================
def text_to_morse(text):
    result = []

    for char in text.upper():
        if char in MORSE_TABLE:
            result.append(MORSE_TABLE[char])

    return "   ".join(result)


# =========================
# 傳送資料給 STM32
# =========================
def send_text():
    text = entry.get().strip()

    # 沒有輸入
    if text == "":
        messagebox.showwarning(
            "提醒",
            "請先輸入英文單字"
        )
        return

    # 目前只接受 A ~ Z
    if not text.isalpha():
        messagebox.showerror(
            "輸入錯誤",
            "目前只支援 A～Z 英文字母"
        )
        return

    # UART 未連線
    if not uart_connected:
        messagebox.showerror(
            "UART 錯誤",
            f"無法連線 {PORT}"
        )
        return

    # 統一轉成大寫
    text = text.upper()

    # 傳給 STM32
    # \n 代表一個完整單字結束
    ser.write((text + "\n").encode("utf-8"))

    # 顯示摩斯碼
    morse = text_to_morse(text)

    morse_label.config(
        text="Morse： " + morse
    )

    # 顯示狀態
    status_label.config(
        text="狀態：已送出 " + text
    )


# =========================
# Enter 也可以傳送
# =========================
def press_enter(event):
    send_text()


# =========================
# 關閉視窗
# =========================
def close_window():
    if ser is not None and ser.is_open:
        ser.close()

    root.destroy()


# =========================
# 建立 GUI
# =========================
root = tk.Tk()

root.title("光語密碼：STM32 摩斯碼播放器")

root.geometry("520x360")

root.resizable(False, False)

root.protocol(
    "WM_DELETE_WINDOW",
    close_window
)


# =========================
# 標題
# =========================
title_label = tk.Label(
    root,
    text="光語密碼：STM32 摩斯碼播放器",
    font=("Microsoft JhengHei", 20, "bold")
)

title_label.pack(
    pady=(25, 15)
)


# =========================
# UART 狀態
# =========================
if uart_connected:

    uart_label = tk.Label(
        root,
        text=f"UART：{PORT} 已連線",
        font=("Microsoft JhengHei", 12)
    )

else:

    uart_label = tk.Label(
        root,
        text=f"UART：{PORT} 連線失敗",
        font=("Microsoft JhengHei", 12)
    )

uart_label.pack(
    pady=5
)


# =========================
# 英文輸入區
# =========================
input_frame = tk.Frame(root)

input_frame.pack(
    pady=20
)


input_label = tk.Label(
    input_frame,
    text="輸入英文：",
    font=("Microsoft JhengHei", 13)
)

input_label.pack(
    side="left"
)


entry = tk.Entry(
    input_frame,
    width=24,
    font=("Arial", 14)
)

entry.pack(
    side="left",
    padx=10
)

entry.bind(
    "<Return>",
    press_enter
)


# =========================
# 傳送按鈕
# =========================
send_button = tk.Button(
    root,
    text="傳送",
    width=14,
    height=2,
    font=("Microsoft JhengHei", 12),
    command=send_text
)

send_button.pack(
    pady=10
)


# =========================
# Morse 顯示
# =========================
morse_label = tk.Label(
    root,
    text="Morse：",
    font=("Courier New", 14),
    wraplength=450
)

morse_label.pack(
    pady=20
)


# =========================
# 狀態顯示
# =========================
status_label = tk.Label(
    root,
    text="狀態：等待輸入",
    font=("Microsoft JhengHei", 12)
)

status_label.pack(
    pady=5
)


# 游標直接放進輸入框
entry.focus()


# =========================
# 啟動 GUI
# =========================
root.mainloop()
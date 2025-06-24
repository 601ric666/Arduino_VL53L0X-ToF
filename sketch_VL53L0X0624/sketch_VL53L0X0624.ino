#include <Wire.h>
#include "Adafruit_VL53L0X.h"

// VL53L0X 感測器物件
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// 距離範圍設定（單位：公分）
const int TABLET1_WIN_DISTANCE_MIN = 5;
const int TABLET1_WIN_DISTANCE_MAX = 13;
const int TABLET2_WIN_DISTANCE_MIN = 21;
const int TABLET2_WIN_DISTANCE_MAX = 37;

// 防止重複觸發的設定
bool gameActive = true;
unsigned long lastTriggerTime = 0;
const unsigned long DEBOUNCE_DELAY = 2000;

// 穩定性檢測
const int STABLE_READINGS = 3;
int stableCount = 0;
long lastStableDistance = -1;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!lox.begin()) {
    Serial.println("❌ 未偵測到 VL53L0X 感測器，請檢查接線！");
    while (1);
  }

  Serial.println("✅ VL53L0X 啟動完成");
  Serial.println("=== 單 ToF 感測器遊戲控制器 ===");
  Serial.println("距離範圍設定:");
  Serial.print("Tablet1 獲勝: ");
  Serial.print(TABLET1_WIN_DISTANCE_MIN);
  Serial.print("-");
  Serial.print(TABLET1_WIN_DISTANCE_MAX);
  Serial.println(" cm");
  Serial.print("Tablet2 獲勝: ");
  Serial.print(TABLET2_WIN_DISTANCE_MIN);
  Serial.print("-");
  Serial.print(TABLET2_WIN_DISTANCE_MAX);
  Serial.println(" cm");
  Serial.println("遊戲開始！");
}

long readDistanceCM() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {
    long distance = measure.RangeMilliMeter / 10;  // mm → cm
    if (distance < 1 || distance > 200) return -1;
    return distance;
  } else {
    return -1;
  }
}

bool isInRange(long distance, int minDist, int maxDist) {
  return (distance >= minDist && distance <= maxDist);
}

void checkWinCondition(long distance) {
  unsigned long currentTime = millis();
  if (currentTime - lastTriggerTime < DEBOUNCE_DELAY) return;

  if (distance == lastStableDistance) {
    stableCount++;
  } else {
    stableCount = 1;
    lastStableDistance = distance;
  }

  if (stableCount < STABLE_READINGS) return;

  if (gameActive) {
    if (isInRange(distance, TABLET1_WIN_DISTANCE_MIN, TABLET1_WIN_DISTANCE_MAX)) {
      Serial.println("🎉 TABLET1_WIN");
      Serial.print("觸發距離: ");
      Serial.print(distance);
      Serial.println(" cm");
      gameActive = false;
      lastTriggerTime = currentTime;
    } else if (isInRange(distance, TABLET2_WIN_DISTANCE_MIN, TABLET2_WIN_DISTANCE_MAX)) {
      Serial.println("🎉 TABLET2_WIN");
      Serial.print("觸發距離: ");
      Serial.print(distance);
      Serial.println(" cm");
      gameActive = false;
      lastTriggerTime = currentTime;
    }
  }
}

void resetGame() {
  gameActive = true;
  stableCount = 0;
  lastStableDistance = -1;
  Serial.println("🔄 遊戲重置 - 準備下一輪");
}

void loop() {
  long distance = readDistanceCM();

  // 顯示距離
  static unsigned long lastDisplayTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastDisplayTime >= 1000) {
    if (distance != -1) {
      Serial.print("距離: ");
      Serial.print(distance);
      Serial.print(" cm");
      if (!gameActive) {
        Serial.print(" [遊戲暫停]");
      } else if (isInRange(distance, TABLET1_WIN_DISTANCE_MIN, TABLET1_WIN_DISTANCE_MAX)) {
        Serial.print(" [Tablet1 範圍]");
      } else if (isInRange(distance, TABLET2_WIN_DISTANCE_MIN, TABLET2_WIN_DISTANCE_MAX)) {
        Serial.print(" [Tablet2 範圍]");
      }
      Serial.println();
    } else {
      Serial.println("距離: 無效讀數");
    }
    lastDisplayTime = currentTime;
  }

  if (distance != -1) {
    checkWinCondition(distance);
  }

  // 接收序列指令
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();
    command.toUpperCase();

    if (command == "RESET" || command == "RESTART") {
      resetGame();
    } else if (command == "TEST") {
      Serial.println("=== 測試模式 ===");
      Serial.print("當前距離: ");
      Serial.print(distance);
      Serial.println(" cm");
      Serial.print("Tablet1 範圍: ");
      Serial.print(TABLET1_WIN_DISTANCE_MIN);
      Serial.print("-");
      Serial.print(TABLET1_WIN_DISTANCE_MAX);
      Serial.println(" cm");
      Serial.print("Tablet2 範圍: ");
      Serial.print(TABLET2_WIN_DISTANCE_MIN);
      Serial.print("-");
      Serial.print(TABLET2_WIN_DISTANCE_MAX);
      Serial.println(" cm");
      Serial.print("遊戲狀態: ");
      Serial.println(gameActive ? "進行中" : "暫停");
    }
  }

  if (!gameActive && (currentTime - lastTriggerTime > 10000)) {
    resetGame();
  }

  delay(100);
}

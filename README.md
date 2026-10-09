Physical Computing Project 2026 - IT KMITL

# S.E.A.T. (Smart Ergonomic & Available Table)
> **ระบบจัดการที่นั่งห้องสมุดอัจฉริยะและแจ้งเตือนป้องกันออฟฟิศซินโดรม**

---

## สมาชิกในกลุ่ม (Team Members)

| รหัสนักศึกษา | ชื่อ-นามสกุล | หน้าที่ / บทบาทหลัก |

| 68070099 | นายปองคุณ อิ่มไชย | System Tester & Documentation |
| 68070128 | นายพีระพัฒน์ เสียนาสระ | Backend & Cloud API Developer |
| 68070131 | นายภควรรษ หันหลวงราช | Embedded Firmware & Hardware Engineer |
| 68070148 | นายภูสิทธิ์ รุ่งเรือง | Frontend Web Developer |
| 68070164 | นายวชิรวิทย์ อนุสุนชนัง | Hardware Assembly & Integration |
| 68070169 | นายวราเทพ ภานนท์ | UI/UX & Web Developer |
| 68070179 | นายศุภวิชญ์ สุขจันทร์ | Database Administrator & Testing |

*คณะเทคโนโลยีสารสนเทศ สถาบันเทคโนโลยีพระจอมเกล้าเจ้าคุณทหารลาดกระบัง (IT KMITL)*

---

## จุดประสงค์ของโปรเจกต์ (Objectives)

1. **อำนวยความสะดวกในการค้นหาที่นั่ง:** ลดเวลาเดินวนหาโต๊ะอ่านหนังสือของนักศึกษาและผู้ใช้บริการห้องสมุด โดยเฉพาะช่วงใกล้สอบ
2. **ขจัดปัญหาการกั๊กโต๊ะ:** แก้ปัญหาการนำสิ่งของมาวางจองที่นั่งไว้แต่ตัวไม่อยู่ ทำให้ผู้อื่นเสียโอกาส
3. **ส่งเสริมสุขภาพผู้ใช้งาน:** มีระบบแจ้งเตือน Active Reminder ให้ผู้ใช้เปลี่ยนอิริยาบถเพื่อป้องกันโรคออฟฟิศซินโดรม
4. **สรุปและวิเคราะห์ข้อมูล:** จัดเก็บและทำรายงานสถิติการใช้งานที่นั่งเพื่อช่วยบริหารจัดการพื้นที่ห้องสมุด

---

## รายละเอียดโปรเจกต์ (Project Details)

โปรเจกต์นี้เป็นการพัฒนาระบบบริหารจัดการที่นั่งในห้องสมุดแบบครบวงจร ผสานเทคโนโลยี IoT เข้ากับการดูแลสุขภาพ โดยมีฟังก์ชันหลัก ดังนี้:

* **Physical Verification & Auto-Release:** ใช้ **Ultrasonic Sensor** ตรวจจับการนั่งจริงหากลุกออกจากโต๊ะเกินเวลาที่กำหนด ระบบจะทำการ **"คืนที่นั่งอัตโนมัติ"** เพื่อป้องกันการวางของกั๊กโต๊ะ
* **Real-time Seat Map via QR Code:** ผู้ใช้สามารถสแกน QR Code ประจำจุดเพื่อเปิดดูผังที่นั่งว่างผ่าน Web Browser ได้ทันที ไม่ต้องโหลดแอปพลิเคชันเพิ่มเติม
* **Office Syndrome Active Reminder:** ระบบจับเวลาการนั่งติดต่อกันและส่งสัญญาณเตือน (ผ่านหน้าจอแสดงผลและ Buzzer) ให้ลุกเปลี่ยนอิริยาบถ สามารถกดตั้งค่าหรือควบคุมได้ง่ายผ่าน **Keypad** หน้าโต๊ะ
* **Privacy & Cost-Effective:** ใช้เซนเซอร์ตรวจจับแทนกล้องวงจรปิด ไม่ละเมิดความเป็นส่วนตัว และมีต้นทุนอุปกรณ์ต่ำ
* **Usage Analytics:** จัดเก็บบันทึกและสรุปรายงานสถิติการเข้า-ออก เพื่อให้นำไปวิเคราะห์การใช้งานพื้นที่ได้

---

## อุปกรณ์และเทคโนโลยีที่ใช้ (Tech Stack & Hardware)

### **Hardware Components**
* **Microcontroller:** ESP32 (Wi-Fi Enabled)
* **Sensors & Input:** Ultrasonic Sensor (HC-SR04), 4x4 Matrix Keypad
* **Display & Output:** LCD1602 Display (I2C), Buzzer Speaker

### **Software & Libraries**
* **Firmware:** Arduino IDE (C++), `WiFi.h`, `Keypad.h`, `LiquidCrystal_I2C.h`
* **Frontend:** HTML5, CSS3, JavaScript (Real-time Seat Map UI)
* **Backend & Database:** Node.js / Express, Firebase / MySQL

/* แก้ลิงก์ รูป และสมาชิกได้ที่นี่ ไม่ต้องแก้ script.js
   รูป: วางไฟล์ใน assets/ แล้วใส่ชื่อ เช่น "assets/prototype.jpg"
   ช่องที่ยังไม่มีข้อมูลให้เว้นเป็น "" */
window.PROJECT_CONTENT = {
  links: {
    website: "../index.html", // หน้า SEAT-SENSE Website เดิมใน repo
    github: "https://github.com/Kmitl68070131/SEAT-SENSE",
    video: "", // วาง URL วิดีโอนำเสนอ
    poster: "https://drive.google.com/file/d/1i8Fl24YtK2P3wG4ywaIeZ6JDMK3-GOQW/view?usp=sharing" // เช่น "assets/poster.pdf" หรือ URL ไฟล์ PDF
  },
  images: {
    hero: "pic/mainjra (2).png", // รูปชิ้นงานหลัก
    hardware1: "pic/gear1.jpg", hardware2: "pic/gear2.jpg", hardware3: "pic/gear3.jpg", hardware4: "pic/gear4.jpg",
    hardware5: "pic/keypad.jpg", // รูป Keypad 4×4 เช่น "pic/gear5.jpg"
    hardware6: "pic/led.jpg", // รูป RGB LED เช่น "pic/gear6.jpg"
    hardware7: "pic/tmp36.jpg", // รูป TMP36 Temperature Sensor เช่น "pic/gear7.jpg"
    prototype: "pic/prototype-new.png", // รูปชิ้นงานจริง
    website: "pic/website-full.png", // รูปหน้าจอ SEAT-SENSE Website
    timer: "pic/timer-settime.jpg", // รูปฟีเจอร์การตั้งเวลา เช่น "pic/set-time.jpg"
    moveAround: "pic/timer-expired.jpg", // รูปไฟสีแดงแจ้งเตือน Move Around เช่น "pic/move-around.jpg"
    temperature: "pic/7. A_roomtemp (1).jpg" // รูป LCD แสดงอุณหภูมิ เช่น "pic/temp-result.jpg"
  },
  members: [
  {
    name: "นายพีระพัฒน์ เสียนาสระ",
    id: "68070128",
    role: "Project Website & Poster Design",
    image: "pic/member1.jpg"
  },
  {
    name: "นายภควรรษ หันหลวงราช",
    id: "68070131",
    role: "Embedded Firmware & Hardware Engineer",
    image: "pic/1753347529672.jpg"
  },
  {
    name: "นายภูสิทธิ์ รุ่งเรือง",
    id: "68070148",
    role: "Project Website & Poster Design",
    image: "pic/member3.jpg"
  },
  {
    name: "นายวชิรวิทย์ อนุสุนชนัง",
    id: "68070164",
    role: "Embedded Firmware & Hardware Engineer",
    image: "pic/member4.jpg"
  }
  ]
};

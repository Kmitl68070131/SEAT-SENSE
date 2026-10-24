/* แก้ลิงก์ รูป และสมาชิกได้ที่นี่ ไม่ต้องแก้ script.js
   รูป: วางไฟล์ใน assets/ แล้วใส่ชื่อ เช่น "assets/prototype.jpg"
   ช่องที่ยังไม่มีข้อมูลให้เว้นเป็น "" */
window.PROJECT_CONTENT = {
  links: {
    website: "../index.html", // หน้า SmartDesk เดิมใน repo
    github: "https://github.com/Kmitl68070131/SEAT-SENSE",
    video: "", // วาง URL คลิปพรีเซนต์
    poster: "" // เช่น "assets/poster.pdf" หรือ URL ไฟล์ PDF
  },
  images: {
    hero: "pic/mainjra.png", // รูปชิ้นงานหลัก
    hardware1: "pic/gear1.jpg", hardware2: "pic/gear2.jpg", hardware3: "pic/gear3.jpg", hardware4: "pic/gear4.jpg",
    diagram: "", // แผนภาพระบบ
    prototype: "pic/resu1.jpg", // รูปชิ้นงานจริง
    website: "pic/resu2.jpg" // รูปหน้าจอ SmartDesk
  },
  members: [
  {
    name: "นายพีระพัฒน์ เสียนาสระ",
    id: "68070128",
    role: "Backend & Cloud API Developer",
    image: "pic/member1.jpg"
  },
  {
    name: "นายภควรรษ หันหลวงราช",
    id: "68070131",
    role: "Embedded Firmware & Hardware Engineer",
    image: "pic/member2.jpg"
  },
  {
    name: "นายภูสิทธิ์ รุ่งเรือง",
    id: "68070148",
    role: "Frontend Web Developer",
    image: "pic/member3.jpg"
  },
  {
    name: "นายวชิรวิทย์ อนุสุนชนัง",
    id: "68070164",
    role: "Hardware Assembly & Integration",
    image: "pic/member4.jpg"
  }
  ]
};

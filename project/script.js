(() => {
  'use strict';
  const content = window.PROJECT_CONTENT || {};
  // อนุญาตลิงก์เว็บและไฟล์ในโปรเจกต์ ป้องกันการใส่ javascript: โดยไม่ตั้งใจ
  function safeURL(value) {
    if (typeof value !== 'string' || !value.trim()) return '';
    try {
      const url = new URL(value.trim(), location.href);
      return ['http:', 'https:', 'file:'].includes(url.protocol) ? value.trim() : '';
    } catch { return ''; }
  }
  document.querySelectorAll('[data-link]').forEach(link => {
    const url = safeURL(content.links?.[link.dataset.link]);
    if (!url) {
      link.removeAttribute('href');
      link.setAttribute('aria-disabled', 'true');
      link.classList.add('pending');
      return;
    }
    link.href = url;
    link.removeAttribute('aria-disabled');
    link.classList.remove('pending');
    // เปิดระบบและสื่อต่าง ๆ ในแท็บใหม่ เพื่อกลับมาอ่านรายละเอียดต่อได้
    link.target = '_blank';
    link.rel = 'noopener noreferrer';
    const status = link.querySelector('.resource-status');
    if (status) status.textContent = ({video: 'ดูคลิปพรีเซนต์ ↗', poster: 'เปิดโปสเตอร์ PDF ↗', website: 'เปิดเว็บไซต์ ↗', github: 'เปิด GitHub ↗'})[link.dataset.link];
  });
  function attachImage(slot, path, alt) {
    const url = safeURL(path);
    if (!url) return;
    const img = new Image();
    img.alt = alt;
    img.loading = slot.dataset.image === 'hero' ? 'eager' : 'lazy';
    // คงช่องรูปเดิมไว้ถ้าพาธผิดหรือโหลดรูปไม่ได้
    img.addEventListener('load', () => {
      slot.replaceChildren(img);
      slot.classList.add('has-image');
    }, {once: true});
    img.src = url;
  }
  document.querySelectorAll('[data-image]').forEach(slot => {
    attachImage(slot, content.images?.[slot.dataset.image], slot.innerText.trim() || 'รูปประกอบโครงงาน SEAT-SENSE');
  });
  const team = document.getElementById('team-list');
  (content.members || []).forEach((member, index) => {
    const card = document.createElement('article');
    const photo = document.createElement('div');
    photo.className = 'member-photo';
    photo.innerHTML = '<span class="member-index">MEMBER ' + String(index + 1).padStart(2, '0') + '</span><svg viewBox="0 0 100 100" aria-hidden="true"><circle cx="50" cy="33" r="16" fill="none" stroke="currentColor" stroke-width="2"/><path d="M20 83c0-35 60-35 60 0" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"/></svg>';
    card.append(photo);
    [['h3', 'member-name', member.name], ['p', 'member-id', member.id], ['p', 'member-role', member.role]].forEach(([tag, className, value]) => {
      const el = document.createElement(tag);
      el.className = className;
      el.textContent = value || '';
      card.append(el);
    });
    team.append(card);
    attachImage(photo, member.image, member.name || 'รูปสมาชิก');
  });
  const toggle = document.querySelector('.menu-toggle');
  const nav = document.getElementById('navigation');
  toggle.addEventListener('click', () => {
    const open = toggle.getAttribute('aria-expanded') !== 'true';
    toggle.setAttribute('aria-expanded', String(open));
    nav.classList.toggle('open', open);
  });
  function closeMenu() {
    toggle.setAttribute('aria-expanded', 'false');
    nav.classList.remove('open');
  }
  nav.querySelectorAll('a').forEach(a => a.addEventListener('click', closeMenu));
  document.addEventListener('keydown', e => { if (e.key === 'Escape') closeMenu(); });
})();

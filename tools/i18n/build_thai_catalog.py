#!/usr/bin/env python3
"""
tools/i18n/build_thai_catalog.py
Builds the complete, polished Thai translation catalog for PoseStudio.
Translates all 282 extracted messages plus standard Qt button contexts.
"""

import os
import xml.etree.ElementTree as ET

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TS_PATH = os.path.join(ROOT, "resources", "translations", "posestudio_th.ts")

# Master translation dictionary: (context, source) -> translation
TRANSLATIONS = {
    # --- AssetBreadcrumbLabel ---
    ("AssetBreadcrumbLabel", "Select a folder to view assets..."): "เลือกโฟลเดอร์เพื่อดูแอสเซท...",
    ("AssetBreadcrumbLabel", "Open"): "เปิด",
    ("AssetBreadcrumbLabel", "Browse Folder"): "เปิดดูโฟลเดอร์",

    # --- AssetManagerWidget ---
    ("AssetManagerWidget", "Search assets..."): "ค้นหาแอสเซท...",
    ("AssetManagerWidget", '<a href="#" style="color: %1;">%2</a>'): '<a href="#" style="color: %1;">%2</a>',
    ("AssetManagerWidget", "Add Asset Folder"): "เพิ่มโฟลเดอร์แอสเซท",
    ("AssetManagerWidget", "Select Asset Library Folder"): "เลือกโฟลเดอร์คลังแอสเซท",
    ("AssetManagerWidget", "Search Results"): "ผลการค้นหา",
    ("AssetManagerWidget", "Favorites"): "รายการโปรด",
    ("AssetManagerWidget", "Collections"): "คอลเลกชัน",
    ("AssetManagerWidget", "Searching..."): "กำลังค้นหา...",
    ("AssetManagerWidget", "(No results)"): "(ไม่พบผลลัพธ์)",
    ("AssetManagerWidget", "Assets: %1"): "แอสเซท: %1",
    ("AssetManagerWidget", "Folders: %1"): "โฟลเดอร์: %1",
    ("AssetManagerWidget", "Sortable"): "จัดเรียงได้",
    ("AssetManagerWidget", "Collection"): "คอลเลกชัน",
    ("AssetManagerWidget", "File"): "ไฟล์",
    ("AssetManagerWidget", ".%1 File"): "ไฟล์ .%1",
    ("AssetManagerWidget", "New Collection"): "สร้างคอลเลกชันใหม่",
    ("AssetManagerWidget", "Manage Asset Folders"): "จัดการโฟลเดอร์แอสเซท",
    ("AssetManagerWidget", "Refresh"): "รีเฟรช",
    ("AssetManagerWidget", "Expand"): "ขยาย",
    ("AssetManagerWidget", "Expand Branch"): "ขยายกิ่งนี้",
    ("AssetManagerWidget", "Collapse"): "ยุบ",
    ("AssetManagerWidget", "Find In Library"): "ค้นหาในคลังแอสเซท",
    ("AssetManagerWidget", "Browse Folder"): "เปิดดูโฟลเดอร์",
    ("AssetManagerWidget", "Add To Collection"): "เพิ่มไปยังคอลเลกชัน",
    ("AssetManagerWidget", "Rename %1"): "เปลี่ยนชื่อ %1",
    ("AssetManagerWidget", "Delete %1"): "ลบ %1",
    ("AssetManagerWidget", "New Sub-Collection"): "สร้างคอลเลกชันย่อยใหม่",
    ("AssetManagerWidget", "Open"): "เปิด",
    ("AssetManagerWidget", "Add To Favorites"): "เพิ่มในรายการโปรด",
    ("AssetManagerWidget", "Move To Collection"): "ย้ายไปยังคอลเลกชัน",
    ("AssetManagerWidget", "Copy To Collection"): "คัดลอกไปยังคอลเลกชัน",
    ("AssetManagerWidget", "Remove From Collection"): "นำออกจากคอลเลกชัน",
    ("AssetManagerWidget", "Remove From Favorites"): "นำออกจากรายการโปรด",

    # --- AssetsPreferencesPanel ---
    ("AssetsPreferencesPanel", "Assets"): "แอสเซท",
    ("AssetsPreferencesPanel", "Asset folders are scanned for 3D models, poses, and other assets. Add the root folder of each library you want to browse in the Asset Manager. Double-click a folder to jump to it."): "ระบบจะสแกนโฟลเดอร์แอสเซทเพื่อหาโมเดล 3D ท่าโพส และแอสเซทอื่นๆ เพิ่มโฟลเดอร์หลักของแต่ละคลังที่คุณต้องการเปิดดูในตัวจัดการแอสเซท ดับเบิลคลิกที่โฟลเดอร์เพื่อข้ามไปยังโฟลเดอร์นั้น",
    ("AssetsPreferencesPanel", "Add Asset Folder..."): "เพิ่มโฟลเดอร์แอสเซท...",
    ("AssetsPreferencesPanel", "Remove Selected"): "ลบรายการที่เลือก",
    ("AssetsPreferencesPanel", "Select Asset Library Folder"): "เลือกโฟลเดอร์คลังแอสเซท",
    ("AssetsPreferencesPanel", "Already Added"): "เพิ่มไว้แล้ว",
    ("AssetsPreferencesPanel", "That folder is already registered as an asset library:\n%1"): "โฟลเดอร์นี้ได้รับการลงทะเบียนเป็นคลังแอสเซทอยู่แล้ว:\n%1",

    # --- FactoryResetPreferencesPanel ---
    ("FactoryResetPreferencesPanel", "Factory Reset"): "รีเซ็ตเป็นค่าเริ่มต้นจากโรงงาน",
    ("FactoryResetPreferencesPanel", "Permanently deletes all your asset libraries, collections, and preferences. This cannot be undone. PoseStudio will restart automatically once the reset is complete."): "ลบคลังแอสเซท คอลเลกชัน และการตั้งค่าทั้งหมดของคุณอย่างถาวร การกระทำนี้ไม่สามารถย้อนกลับได้ PoseStudio จะเริ่มทำงานใหม่โดยอัตโนมัติเมื่อการรีเซ็ตเสร็จสมบูรณ์",
    ("FactoryResetPreferencesPanel", 'To confirm, type RESET into the box below, then click "Factory Reset".'): 'เพื่อยืนยัน ให้พิมพ์ RESET ลงในช่องด้านล่าง แล้วคลิก "รีเซ็ตเป็นค่าเริ่มต้นจากโรงงาน"',
    ("FactoryResetPreferencesPanel", "Type RESET to confirm"): "พิมพ์ RESET เพื่อยืนยัน",

    # --- GeneralPreferencesPanel ---
    ("GeneralPreferencesPanel", "General"): "ทั่วไป",
    ("GeneralPreferencesPanel", "Send an anonymous install ping"): "ส่งสัญญาณสถิติการติดตั้งแบบไม่ระบุตัวตน",
    ("GeneralPreferencesPanel", "Each time it starts, PoseStudio lets posestudio.io know that this installation exists, so we can see how many people use it and which versions are out there. The ping carries a random install ID, the app version, your operating system and CPU type, whether the app was installed or is running portable, the Qt library version, and the time of the ping — nothing else: no names, no files, no usage data, and the ID isn't linked to you in any way."): "ทุกครั้งที่เปิดโปรแกรม PoseStudio จะแจ้งให้ posestudio.io ทราบว่ามีการติดตั้งนี้อยู่ เพื่อให้เราทราบจำนวนผู้ใช้งานและเวอร์ชันที่มีการใช้งาน ข้อมูลที่ส่งประกอบด้วย ID การติดตั้งแบบสุ่ม เวอร์ชันของโปรแกรม ระบบปฏิบัติการและประเภทซีพียู สถานะการติดตั้งหรือรันแบบพกพา เวอร์ชันไลบรารี Qt และเวลาที่ส่ง โดยไม่มีข้อมูลชื่อ ไฟล์ หรือพฤติกรรมการใช้งาน และ ID จะไม่เชื่อมโยงกับตัวตนของคุณแต่อย่างใด",
    ("GeneralPreferencesPanel", "Install ID: not created yet (assigned when the first ping is sent)"): "ID การติดตั้ง: ยังไม่ได้สร้าง (จะถูกกำหนดเมื่อส่งสัญญาณครั้งแรก)",
    ("GeneralPreferencesPanel", "Install ID: %1"): "ID การติดตั้ง: %1",
    ("GeneralPreferencesPanel", "This build has no ping key, so nothing is sent regardless of this setting."): "บิลด์นี้ไม่มีคีย์รับรอง จึงไม่มีการส่งข้อมูลใดๆ ไม่ว่าจะตั้งค่าอย่างไร",
    ("GeneralPreferencesPanel", "Check for updates at startup"): "ตรวจสอบการอัปเดตเมื่อเปิดโปรแกรม",
    ("GeneralPreferencesPanel", "A few seconds after it starts, PoseStudio asks GitHub for the latest release and, when a newer one exists, shows a message with a link to its download page. Nothing but the request itself is sent. Help → Check for Updates… does the same on demand."): "หลังจากเปิดโปรแกรมสองสามวินาที PoseStudio จะตรวจสอบเวอร์ชันล่าสุดจาก GitHub และแจ้งเตือนพร้อมลิงก์ดาวน์โหลดเมื่อมีเวอร์ชันใหม่ ทั้งนี้ไม่มีการส่งข้อมูลอื่นใดนอกเหนือจากการตรวจสอบเวอร์ชัน และคุณสามารถกด Help → ตรวจสอบการอัปเดต… ได้ด้วยตนเองตลอดเวลา",
    ("GeneralPreferencesPanel", "Count camera changes as unsaved"): "นับการขยับกล้องเป็นการเปลี่ยนแปลงที่ยังไม่ได้บันทึก",
    ("GeneralPreferencesPanel", "When on, moving the camera (orbit, pan, zoom, the view hotkeys) marks the scene as having unsaved changes — the close prompt and File → New will ask before discarding it. Off by default: looking around is not an edit."): "เมื่อเปิดใช้งาน การขยับกล้อง (หมุนมุมมอง, แพน, ซูม หรือปุ่มลัดมุมมอง) จะทำให้ฉากถูกระบุว่ามีการเปลี่ยนแปลงที่ยังไม่ได้บันทึก โดยจะมีข้อความถามก่อนปิดโปรแกรมหรือเมื่อสร้างไฟล์ใหม่ ปิดใช้งานเป็นค่าเริ่มต้นเนื่องจากการเลื่อนดูฉากไม่ใช่การแก้ไขเนื้อหา",

    # --- HelpWindow ---
    ("HelpWindow", "%1 User Manual"): "คู่มือการใช้งาน %1",
    ("HelpWindow", "‹ Back"): "‹ ย้อนกลับ",
    ("HelpWindow", "Back to the previous page (Alt+Left)"): "กลับไปยังหน้าก่อนหน้า (Alt+Left)",
    ("HelpWindow", "Forward ›"): "ไปข้างหน้า ›",
    ("HelpWindow", "Forward again (Alt+Right)"): "ไปยังหน้าถัดไป (Alt+Right)",
    ("HelpWindow", "Search the manual  (Ctrl+F)"): "ค้นหาในคู่มือ  (Ctrl+F)",
    ("HelpWindow", "# User Manual\n\n%1"): "# คู่มือการใช้งาน\n\n%1",

    # --- InterfacePreferencesPanel ---
    ("InterfacePreferencesPanel", "Interface"): "ส่วนติดต่อผู้ใช้",
    ("InterfacePreferencesPanel", "Appearance Theme"): "ธีมหน้าตาโปรแกรม",
    ("InterfacePreferencesPanel", "Dark"): "ธีมมืด (Dark)",
    ("InterfacePreferencesPanel", "Light"): "ธีมสว่าง (Light)",
    ("InterfacePreferencesPanel", "The original dark look"): "หน้าตาธีมสีเข้มแบบดั้งเดิม",
    ("InterfacePreferencesPanel", "A light look with the same layout"): "หน้าตาธีมสีสว่างสบายตาในโครงสร้างเดิม",
    ("InterfacePreferencesPanel", "Chooses the application's look. The change applies immediately and is remembered between launches."): "เลือกหน้าตาของโปรแกรม การเปลี่ยนแปลงมีผลทันทีและจะจำค่าไว้เมื่อเปิดใช้งานครั้งถัดไป",
    ("InterfacePreferencesPanel", "Language"): "ภาษา (Language)",
    ("InterfacePreferencesPanel", "Chooses the interface display language. The change is remembered between launches. A restart of PoseStudio is recommended for all menus and windows to refresh completely."): "เลือกภาษาที่แสดงในโปรแกรม การเปลี่ยนแปลงจะจำค่าไว้เมื่อเปิดใช้งานครั้งถัดไป แนะนำให้เริ่มโปรแกรมใหม่เพื่อให้เมนูและหน้าต่างทั้งหมดรีเฟรชอย่างสมบูรณ์",

    # --- MenuManager ---
    ("MenuManager", "File"): "ไฟล์",
    ("MenuManager", "New"): "สร้างใหม่",
    ("MenuManager", "Open..."): "เปิด...",
    ("MenuManager", "Open Recent..."): "เปิดไฟล์ล่าสุด...",
    ("MenuManager", "Save"): "บันทึก",
    ("MenuManager", "Save As..."): "บันทึกเป็น...",
    ("MenuManager", "Save Copy..."): "บันทึกสำเนา...",
    ("MenuManager", "Import"): "นำเข้า",
    ("MenuManager", "Export..."): "ส่งออก...",
    ("MenuManager", "Save Pose..."): "บันทึกท่าโพส...",
    ("MenuManager", "Load Pose..."): "โหลดท่าโพส...",
    ("MenuManager", "Quit"): "ออก",
    ("MenuManager", "Edit"): "แก้ไข",
    ("MenuManager", "Undo"): "เลิกทำ",
    ("MenuManager", "Redo"): "ทำซ้ำ",
    ("MenuManager", "Undo History..."): "ประวัติการเลิกทำ...",
    ("MenuManager", "Copy"): "คัดลอก",
    ("MenuManager", "Paste"): "วาง",
    ("MenuManager", "Delete Selected Object\tDel"): "ลบออบเจกต์ที่เลือก\tDel",
    ("MenuManager", "Reset Selected Joint"): "รีเซ็ตข้อต่อที่เลือก",
    ("MenuManager", "Reset Limb"): "รีเซ็ตระยางค์ส่วนแขนขา",
    ("MenuManager", "Reset Pose"): "รีเซ็ตท่าโพส",
    ("MenuManager", "Turn Joint with Mouse"): "หมุนข้อต่อด้วยเมาส์",
    ("MenuManager", "Bend"): "โค้งงอ (Bend)",
    ("MenuManager", "Side-Side"): "ซ้าย-ขวา (Side-Side)",
    ("MenuManager", "Twist"): "บิดหมุน (Twist)",
    ("MenuManager", "Mirror Pose"): "กลับด้านท่าโพส (Mirror)",
    ("MenuManager", "Mirror Limb to Other Side"): "คัดลอกแขนขาสลับด้าน",
    ("MenuManager", "Preferences"): "การตั้งค่า",
    ("MenuManager", "View"): "มุมมอง",
    ("MenuManager", "Show Skeleton"): "แสดงโครงกระดูก",
    ("MenuManager", "Front View"): "มุมมองด้านหน้า",
    ("MenuManager", "Back View"): "มุมมองด้านหลัง",
    ("MenuManager", "Right View"): "มุมมองด้านขวา",
    ("MenuManager", "Left View"): "มุมมองด้านซ้าย",
    ("MenuManager", "Top View"): "มุมมองด้านบน",
    ("MenuManager", "Bottom View"): "มุมมองด้านล่าง",
    ("MenuManager", "Flip View"): "กลับด้านมุมมอง",
    ("MenuManager", "Frame Selected"): "จัดโฟกัสที่เลือก",
    ("MenuManager", "Home View"): "มุมมองเริ่มต้น (Home)",
    ("MenuManager", "Help"): "ช่วยเหลือ",
    ("MenuManager", "User Manual"): "คู่มือการใช้งาน",
    ("MenuManager", "Release Notes"): "บันทึกการเปิดตัว",
    ("MenuManager", "Tutorials"): "บทเรียนการใช้งาน",
    ("MenuManager", "Support"): "ฝ่ายสนับสนุน",
    ("MenuManager", "Check for Updates..."): "ตรวจสอบการอัปเดต...",
    ("MenuManager", "About PoseStudio"): "เกี่ยวกับ PoseStudio",
    ("MenuManager", "About Qt"): "เกี่ยวกับ Qt",
    ("MenuManager", "Import OBJ"): "นำเข้า OBJ",
    ("MenuManager", "Wavefront OBJ (*.obj)"): "Wavefront OBJ (*.obj)",
    ("MenuManager", "Import Character Figure"): "นำเข้ารูปร่างตัวละคร",
    ("MenuManager", "Figure Files (*.duf *.dsf)"): "ไฟล์รูปร่างตัวละคร (*.duf *.dsf)",
    ("MenuManager", "Save Pose"): "บันทึกท่าโพส",
    ("MenuManager", "Import a character figure before saving a pose."): "กรุณานำเข้ารูปร่างตัวละครก่อนบันทึกท่าโพส",
    ("MenuManager", "Pose Files (*.pose)"): "ไฟล์ท่าโพส (*.pose)",
    ("MenuManager", "Could not write the pose file."): "ไม่สามารถเขียนไฟล์ท่าโพสได้",
    ("MenuManager", "Load Pose"): "โหลดท่าโพส",
    ("MenuManager", "Import a character figure before loading a pose."): "กรุณานำเข้ารูปร่างตัวละครก่อนโหลดท่าโพส",
    ("MenuManager", "Could not read the pose file."): "ไม่สามารถอ่านไฟล์ท่าโพสได้",
    ("MenuManager", "Save Project"): "บันทึกโปรเจกต์",
    ("MenuManager", "PoseStudio Projects (*.pss)"): "โปรเจกต์ PoseStudio (*.pss)",
    ("MenuManager", "Could not write the project file."): "ไม่สามารถเขียนไฟล์โปรเจกต์ได้",
    ("MenuManager", "Unsaved Changes"): "การเปลี่ยนแปลงที่ยังไม่ได้บันทึก",
    ("MenuManager", "The current scene has unsaved changes."): "ฉากปัจจุบันมีการเปลี่ยนแปลงที่ยังไม่ได้บันทึก",
    ("MenuManager", "Discard Changes"): "ละทิ้งการเปลี่ยนแปลง",
    ("MenuManager", "Cancel"): "ยกเลิก",
    ("MenuManager", "Open Project"): "เปิดโปรเจกต์",
    ("MenuManager", "Could not read the project file:\n%1"): "ไม่สามารถอ่านไฟล์โปรเจกต์ได้:\n%1",
    ("MenuManager", "Missing Figure File"): "ไม่พบไฟล์รูปร่างตัวละคร",
    ("MenuManager", "The figure file referenced by this project is missing:"): "ไม่พบไฟล์รูปร่างตัวละครที่โปรเจกต์นี้อ้างอิงถึง:",
    ("MenuManager", "Choose File..."): "เลือกไฟล์...",
    ("MenuManager", "Skip"): "ข้าม",
    ("MenuManager", "Locate Figure File"): "ค้นหาไฟล์รูปร่างตัวละคร",
    ("MenuManager", "Figure Files (*.duf *.dsf *.obj)"): "ไฟล์รูปร่างตัวละคร (*.duf *.dsf *.obj)",
    ("MenuManager", "Skipped missing figure files:\n%1"): "ข้ามไฟล์รูปร่างตัวละครที่ไม่พบ:\n%1",
    ("MenuManager", "Could not load the project:\n%1"): "ไม่สามารถโหลดโปรเจกต์ได้:\n%1",

    # --- PreferencesDialog ---
    ("PreferencesDialog", "Preferences"): "การตั้งค่า",
    ("PreferencesDialog", "Close"): "ปิด",
    ("PreferencesDialog", "General"): "ทั่วไป",
    ("PreferencesDialog", "Interface"): "ส่วนติดต่อผู้ใช้",
    ("PreferencesDialog", "Assets"): "แอสเซท",
    ("PreferencesDialog", "Input"): "การป้อนข้อมูล",
    ("PreferencesDialog", "Keyboard and input settings will appear here."): "การตั้งค่าแป้นพิมพ์และการป้อนข้อมูลจะแสดงที่นี่",
    ("PreferencesDialog", "Navigation"): "การนำทาง",
    ("PreferencesDialog", "Viewport navigation settings will appear here."): "การตั้งค่าการนำทางในวิวพอร์ตจะแสดงที่นี่",
    ("PreferencesDialog", "System"): "ระบบ",
    ("PreferencesDialog", "System, performance, and storage settings will appear here."): "การตั้งค่าระบบ ประสิทธิภาพ และพื้นที่จัดเก็บข้อมูลจะแสดงที่นี่",
    ("PreferencesDialog", "Factory Reset"): "รีเซ็ตเป็นค่าเริ่มต้นจากโรงงาน",

    # --- QObject ---
    ("QObject", " [Debug build — slow imports]"): " [Debug build — นำเข้าช้ากว่าปกติ]",
    ("QObject", "Asset Manager"): "ตัวจัดการแอสเซท",
    ("QObject", "Transform"): "แปลงตำแหน่ง (Transform)",
    ("QObject", "Environment"): "สภาพแวดล้อม",
    ("QObject", "Restore default"): "คืนค่าเริ่มต้น",
    ("QObject", "Content Folder Needed"): "จำเป็นต้องระบุโฟลเดอร์เนื้อหา (Content Folder)",
    ("QObject", "PoseStudio couldn't find the geometry and morph data for:\n%1"): "PoseStudio ไม่พบข้อมูลเรขาคณิตและมอร์ฟสำหรับ:\n%1",
    ("QObject", 'The folder you imported from contains the figure\'s presets but not its "data" folder (where the mesh, skeleton, morphs and skin weights live).\n\nPoint PoseStudio at the figure\'s content folder — the folder that directly contains the "data" folder — and it will be remembered for future imports.'): 'โฟลเดอร์ที่คุณนำเข้ามามีเฉพาะพรีเซ็ตของตัวละครแต่ไม่มีโฟลเดอร์ "data" (ซึ่งเป็นที่เก็บเมช โครงกระดูก มอร์ฟ และน้ำหนักสกิน)\n\nกรุณาระบุโฟลเดอร์เนื้อหาของตัวละครให้ PoseStudio (โฟลเดอร์ที่บรรจุโฟลเดอร์ "data" โดยตรง) แล้วระบบจะจดจำไว้สำหรับการนำเข้าในอนาคต',
    ("QObject", "Locate Content Folder…"): "ค้นหาโฟลเดอร์เนื้อหา…",
    ("QObject", 'Select the Content Folder (the one containing "data")'): 'เลือกโฟลเดอร์เนื้อหา (โฟลเดอร์ที่บรรจุ "data")',
    ("QObject", 'No "data" Folder'): 'ไม่พบโฟลเดอร์ "data"',
    ("QObject", 'The selected folder doesn\'t directly contain a "data" folder, so the figure\'s geometry may still not be found. It has been added anyway — pick the folder that directly contains "data" if the import fails.'): 'โฟลเดอร์ที่เลือกไม่มีโฟลเดอร์ "data" อยู่โดยตรง จึงอาจยังไม่พบข้อมูลเรขาคณิตของตัวละคร ทั้งนี้ระบบได้เพิ่มโฟลเดอร์ไว้แล้ว หากนำเข้าไม่สำเร็จโปรดเลือกโฟลเดอร์ที่มีโฟลเดอร์ "data" บรรจุอยู่โดยตรง',
    ("QObject", "Could not import a character figure from:\n%1\n\nThis file may be a pose, material, or other preset rather than a figure.\n\nDetails: %2"): "ไม่สามารถนำเข้ารูปร่างตัวละครจาก:\n%1\n\nไฟล์นี้อาจเป็นท่าโพส แมทีเรียล หรือพรีเซ็ตอื่นๆ ที่ไม่ใช่โมเดลตัวละคร\n\nรายละเอียด: %2",
    ("QObject", 'Could not import a character figure from:\n%1\n\nThe figure\'s geometry and morph data (its "data" folder) couldn\'t be found. Import again and choose "Locate Content Folder…" to point PoseStudio at the folder that contains "data".\n\nDetails: %2'): 'ไม่สามารถนำเข้ารูปร่างตัวละครจาก:\n%1\n\nไม่พบข้อมูลเรขาคณิตและมอร์ฟของโมเดล (โฟลเดอร์ "data") กรุณานำเข้าใหม่อีกครั้งแล้วเลือก "ค้นหาโฟลเดอร์เนื้อหา…" เพื่อระบุโฟลเดอร์ที่มี "data" อยู่\n\nรายละเอียด: %2',
    ("QObject", "Could not import a character figure from:\n%1\n\nDetails: %2"): "ไม่สามารถนำเข้ารูปร่างตัวละครจาก:\n%1\n\nรายละเอียด: %2",
    ("QObject", "Import Failed"): "การนำเข้าล้มเหลว",
    ("QObject", "Importing Figure"): "กำลังนำเข้ารูปร่างตัวละคร",
    ("QObject", "Reading figure…"): "กำลังอ่านข้อมูลโมเดล…",
    ("QObject", ", %1 zones, %2 pose correctives"): ", %1 โซน, %2 การแก้ไขท่าโพส",
    ("QObject", "Decoding textures…"): "กำลังถอดรหัสพื้นผิว (Textures)…",
    ("QObject", "Baking ambient occlusion…"): "กำลังอบแอมเบียนต์ออคลูชัน (AO)…",
    ("QObject", "Uploading to GPU…"): "กำลังส่งข้อมูลไปยังการ์ดจอ (GPU)…",
    ("QObject", "Importing Model"): "กำลังนำเข้าโมเดล",
    ("QObject", "Reading model file…"): "กำลังอ่านไฟล์โมเดล…",

    # --- UpdateCheck ---
    ("UpdateCheck", "Check for Updates"): "ตรวจสอบการอัปเดต",
    ("UpdateCheck", "PoseStudio couldn't check for updates right now.\n\n%1"): "PoseStudio ไม่สามารถตรวจสอบการอัปเดตได้ในขณะนี้\n\n%1",
    ("UpdateCheck", "PoseStudio couldn't read the release information."): "PoseStudio ไม่สามารถอ่านข้อมูลเวอร์ชันล่าสุดได้",
    ("UpdateCheck", "You are running the latest release (PoseStudio %1)."): "คุณกำลังใช้งานเวอร์ชันล่าสุดอยู่แล้ว (PoseStudio %1)",
    ("UpdateCheck", "Update Available"): "มีการอัปเดตใหม่",
    ("UpdateCheck", '<b>PoseStudio %1 is available.</b><br>You are running %2.<br><br><a href="%3" style="color:%4;">%3</a>'): '<b>มี PoseStudio %1 ให้อัปเดตแล้ว</b><br>เวอร์ชันที่คุณใช้อยู่คือ %2<br><br><a href="%3" style="color:%4;">%3</a>',
    ("UpdateCheck", "Open Download Page"): "เปิดหน้าดาวน์โหลด",
    ("UpdateCheck", "Skip This Version"): "ข้ามเวอร์ชันนี้",
    ("UpdateCheck", "Later"): "ไว้ทีหลัง",

    # --- ViewportStrip ---
    ("ViewportStrip", "Home View"): "มุมมองเริ่มต้น (Home)",
    ("ViewportStrip", "Top View"): "มุมมองด้านบน",
    ("ViewportStrip", "Bottom View"): "มุมมองด้านล่าง",
    ("ViewportStrip", "Front View"): "มุมมองด้านหน้า",
    ("ViewportStrip", "Back View"): "มุมมองด้านหลัง",
    ("ViewportStrip", "Left View"): "มุมมองด้านซ้าย",
    ("ViewportStrip", "Right View"): "มุมมองด้านขวา",
    ("ViewportStrip", "PBR Shaded"): "การเรนเดอร์ PBR",
    ("ViewportStrip", "Texture Shaded"): "แสดงพื้นผิว (Texture)",
    ("ViewportStrip", "Flat Texture Shaded"): "แสดงพื้นผิวแบบแบน (Flat Texture)",
    ("ViewportStrip", "Cartoon Shaded"): "เฉดดิ้งแบบการ์ตูน (Toon)",
    ("ViewportStrip", "Matcap"): "แมตแคป (Matcap)",
    ("ViewportStrip", "Clay Shaded"): "เฉดดิ้งแบบดินเหนียว (Clay)",
    ("ViewportStrip", "Lighting Only"): "แสดงเฉพาะแสงเงา (Lighting Only)",
    ("ViewportStrip", "Silhouette"): "เงาดำซิลูเอท (Silhouette)",
    ("ViewportStrip", "Wireframe"): "โครงลวด (Wireframe)",
    ("ViewportStrip", "Hidden Line Wireframe"): "โครงลวดซ่อนเส้นหลัง (Hidden Line)",
    ("ViewportStrip", "Clay Shaded Wireframe"): "เฉดดินเหนียวพร้อมโครงลวด",
    ("ViewportStrip", "Texture Shaded Wireframe"): "พื้นผิวพร้อมโครงลวด",
    ("ViewportStrip", "Albedo"): "อัลบีโด (Albedo)",
    ("ViewportStrip", "Ambient Occlusion"): "แอมเบียนต์ออคลูชัน (AO)",
    ("ViewportStrip", "Roughness Map"): "แผนที่ความหยาบ (Roughness Map)",
    ("ViewportStrip", "Specular Only"): "เฉพาะไฮไลต์สเปคิวลาร์ (Specular Only)",
    ("ViewportStrip", "Normals"): "เวกเตอร์นอร์มอล (Normals)",
    ("ViewportStrip", "UV Checker"): "ตารางตรวจสอบ UV (UV Checker)",

    # --- pose::EnvironmentPanel ---
    ("pose::EnvironmentPanel", "Environment"): "สภาพแวดล้อม",
    ("pose::EnvironmentPanel", "Rotation°"): "การหมุนองศา°",
    ("pose::EnvironmentPanel", "Backdrop"): "ภาพพื้นหลัง",
    ("pose::EnvironmentPanel", "Blur"): "ความเบลอ",
    ("pose::EnvironmentPanel", "Brightness"): "ความสว่าง",
    ("pose::EnvironmentPanel", "Dome radius"): "รัศมีโดม",
    ("pose::EnvironmentPanel", "Exposure && Tone"): "การเปิดรับแสงและโทน (Exposure & Tone)",
    ("pose::EnvironmentPanel", "Exposure"): "การเปิดรับแสง (Exposure)",
    ("pose::EnvironmentPanel", "ACES filmic tonemap"): "โทนแมป ACES filmic",
    ("pose::EnvironmentPanel", "Image-Based Lighting"): "แสงสว่างจากภาพสภาพแวดล้อม (IBL)",
    ("pose::EnvironmentPanel", "Diffuse"): "แสงฟุ้งกระจาย (Diffuse)",
    ("pose::EnvironmentPanel", "Specular"): "แสงสะท้อนมันวาว (Specular)",
    ("pose::EnvironmentPanel", "Ambient fill"): "แสงแอมเบียนต์เติมเต็ม",
    ("pose::EnvironmentPanel", "Key Light"): "ไฟหลัก (Key Light)",
    ("pose::EnvironmentPanel", "Intensity"): "ความเข้มแสง",
    ("pose::EnvironmentPanel", "Azimuth°"): "มุมกวาด Azimuth°",
    ("pose::EnvironmentPanel", "Elevation°"): "มุมเงย Elevation°",
    ("pose::EnvironmentPanel", "Shadow"): "เงา",
    ("pose::EnvironmentPanel", "Enable shadows"): "เปิดใช้งานเงา",
    ("pose::EnvironmentPanel", "Softness"): "ความนุ่มของเงา",
    ("pose::EnvironmentPanel", "Reach"): "ระยะเงา (Reach)",
    ("pose::EnvironmentPanel", "Skin && Rim"): "ผิวหนังและแสงขอบ (Skin & Rim)",
    ("pose::EnvironmentPanel", "Subsurface"): "แสงทะลุเนื้อผิว (Subsurface)",
    ("pose::EnvironmentPanel", "Rim light"): "แสงขอบริมไลท์ (Rim Light)",
    ("pose::EnvironmentPanel", "Restore All Defaults"): "คืนค่าเริ่มต้นทั้งหมด",
    ("pose::EnvironmentPanel", "HDRI"): "HDRI",
    ("pose::EnvironmentPanel", "Off"): "ปิด",
    ("pose::EnvironmentPanel", "Dome"): "โดมสภาพแวดล้อม",
    ("pose::EnvironmentPanel", "Ground-projected dome: the figure stands on the environment's floor"): "โดมฉายระนาบพื้น: ตัวโมเดลจะยืนอยู่บนพื้นของสภาพแวดล้อม",
    ("pose::EnvironmentPanel", "Mode"): "โหมด",

    # --- pose::HdriSelector ---
    ("pose::HdriSelector", "Procedural Studio"): "สตูดิโอแบบโพรซีเดอรัล (Procedural Studio)",
    ("pose::HdriSelector", "No .hdr or .exr files found"): "ไม่พบไฟล์ .hdr หรือ .exr",
    ("pose::HdriSelector", "Open HDRI Folder…"): "เปิดโฟลเดอร์ HDRI…",

    # --- pose::JointModeBadge ---
    ("pose::JointModeBadge", "Bend"): "โค้งงอ (Bend)",
    ("pose::JointModeBadge", "Side-Side"): "ซ้าย-ขวา (Side-Side)",
    ("pose::JointModeBadge", "Twist"): "บิดหมุน (Twist)",
    ("pose::JointModeBadge", "Move the mouse to turn the selected joint. Click to keep the result; Esc or a right-click puts the joint back."): "เลื่อนเมาส์เพื่อหมุนข้อต่อที่เลือก คลิกเพื่อตกลง; กด Esc หรือคลิกขวาเพื่อคืนค่าเดิม",

    # --- pose::TransformPanel ---
    ("pose::TransformPanel", "Rotation"): "การหมุน",
    ("pose::TransformPanel", "Bend"): "โค้งงอ (Bend)",
    ("pose::TransformPanel", "Side-Side"): "ซ้าย-ขวา (Side-Side)",
    ("pose::TransformPanel", "Twist"): "บิดหมุน (Twist)",
    ("pose::TransformPanel", "Bends the joint in its natural direction. 0 is the rest pose, 100 is fully bent; negative values bend it the other way, as far as the joint allows.\nIn the viewport: press B and move the mouse."): "งอข้อต่อในทิศทางธรรมชาติ ค่า 0 คือท่าพัก ค่า 100 คืองอเต็มที่ ค่าติดลบจะงอไปในทิศทางตรงกันข้ามเท่าที่ข้อต่อขยับได้\nในวิวพอร์ต: กด B แล้วเลื่อนเมาส์",
    ("pose::TransformPanel", "Bends the joint in its secondary direction. 0 is the rest pose, 100 is the joint's limit; negative values bend it the other way, as far as the joint allows.\nIn the viewport: press S and move the mouse."): "งอข้อต่อไปในทิศทางรอง ค่า 0 คือท่าพัก ค่า 100 คือขีดจำกัดของข้อต่อ ค่าติดลบจะงอไปในทิศทางตรงกันข้ามเท่าที่ข้อต่อขยับได้\nในวิวพอร์ต: กด S แล้วเลื่อนเมาส์",
    ("pose::TransformPanel", "Turns the joint about its own length. 0 is the rest pose, 100 is the joint's limit; negative values turn it the other way, as far as the joint allows.\nIn the viewport: press T and move the mouse."): "บิดหมุนข้อต่อรอบแกนความยาว ค่า 0 คือท่าพัก ค่า 100 คือขีดจำกัดของข้อต่อ ค่าติดลบจะหมุนไปอีกทางเท่าที่ข้อต่อขยับได้\nในวิวพอร์ต: กด T แล้วเลื่อนเมาส์",
    ("pose::TransformPanel", "Back to the rest pose"): "กลับสู่ท่าพักเดิม",
    ("pose::TransformPanel", "Click a joint on a figure to select it. Its rotation can then be set here."): "คลิกเลือกข้อต่อบนตัวโมเดล จากนั้นสามารถปรับการหมุนได้ที่นี่",
    ("pose::TransformPanel", "Reset All"): "รีเซ็ตทั้งหมด",
    ("pose::TransformPanel", "Returns all three dials to 0: the selected joint's rest pose"): "คืนค่าปุ่มหมุนทั้งสามกลับเป็น 0: ท่าพักเดิมของข้อต่อที่เลือก",
    ("pose::TransformPanel", "No joint selected"): "ยังไม่ได้เลือกข้อต่อ",

    # --- pose::ViewportStrip ---
    ("pose::ViewportStrip", "Viewport shading mode"): "โหมดเฉดดิ้งวิวพอร์ต",
    ("pose::ViewportStrip", "Camera view"): "มุมมองกล้อง",
    ("pose::ViewportStrip", "Reset to the default view"): "รีเซ็ตเป็นมุมมองเริ่มต้น",
    ("pose::ViewportStrip", "Move the selected figure to the ground"): "ย้ายโมเดลที่เลือกไปแตะระนาบพื้น",
    ("pose::ViewportStrip", "Toggle the skeleton overlay"): "เปิด/ปิด การแสดงโครงกระดูก",
    ("pose::ViewportStrip", "Perspective View"): "มุมมองทัศนมิติ (Perspective)",

    # --- pose::ViewportWidget ---
    ("pose::ViewportWidget", "3D viewport unavailable.\n\nCould not create a Vulkan instance — check that your GPU supports Vulkan and that your graphics drivers are up to date."): "วิวพอร์ต 3D ไม่พร้อมใช้งาน\n\nไม่สามารถสร้างอินสแตนซ์ Vulkan ได้ — โปรดตรวจสอบว่าการ์ดจอของคุณรองรับ Vulkan และติดตั้งไดรเวอร์การ์ดจอรุ่นล่าสุดแล้ว",

    # --- pose::VulkanWindow ---
    ("pose::VulkanWindow", "Unpin Joint\tP"): "ถอนการตรึงข้อต่อ\tP",
    ("pose::VulkanWindow", "Pin Joint\tP"): "ตรึงข้อต่อ\tP",
    ("pose::VulkanWindow", "Unpin All Joints"): "ถอนการตรึงข้อต่อทั้งหมด",
    ("pose::VulkanWindow", "Reset Joint"): "รีเซ็ตข้อต่อ",
    ("pose::VulkanWindow", "Reset Limb"): "รีเซ็ตระยางค์ส่วนแขนขา",
    ("pose::VulkanWindow", "Mirror Limb to Other Side"): "คัดลอกแขนขาสลับด้าน",
    ("pose::VulkanWindow", "Delete"): "ลบ",
}

# Standard Qt button / dialog strings to add for complete UI coverage
STANDARD_QT_STRINGS = {
    "QDialogButtonBox": [
        ("OK", "ตกลง"),
        ("Cancel", "ยกเลิก"),
        ("&Cancel", "&ยกเลิก"),
        ("&OK", "&ตกลง"),
        ("Close", "ปิด"),
        ("&Close", "&ปิด"),
        ("Apply", "นำไปใช้"),
        ("Reset", "รีเซ็ต"),
        ("Restore Defaults", "คืนค่าเริ่มต้น"),
        ("&Yes", "&ใช่"),
        ("&No", "&ไม่ใช่"),
        ("Save", "บันทึก"),
        ("&Save", "&บันทึก"),
        ("Discard", "ละทิ้ง"),
        ("&Discard", "&ละทิ้ง"),
    ],
    "QMessageBox": [
        ("OK", "ตกลง"),
        ("Cancel", "ยกเลิก"),
        ("Yes", "ใช่"),
        ("No", "ไม่ใช่"),
        ("Discard", "ละทิ้ง"),
        ("Apply", "นำไปใช้"),
        ("Close", "ปิด"),
    ],
    "QPlatformTheme": [
        ("OK", "ตกลง"),
        ("Cancel", "ยกเลิก"),
        ("Close", "ปิด"),
        ("Apply", "นำไปใช้"),
        ("Reset", "รีเซ็ต"),
        ("Restore Defaults", "คืนค่าเริ่มต้น"),
        ("Yes", "ใช่"),
        ("No", "ไม่ใช่"),
        ("Save", "บันทึก"),
        ("Discard", "ละทิ้ง"),
    ]
}

def main():
    if not os.path.isfile(TS_PATH):
        print(f"[!] Error: {TS_PATH} not found. Run update_translations.py extract first.")
        return

    tree = ET.parse(TS_PATH)
    root = tree.getroot()
    root.set("language", "th")

    translated_count = 0
    missing_count = 0

    for context_elem in root.findall("context"):
        cname = context_elem.find("name").text
        for msg_elem in context_elem.findall("message"):
            source_elem = msg_elem.find("source")
            if source_elem is None:
                continue
            source_text = source_elem.text
            trans_elem = msg_elem.find("translation")
            if trans_elem is None:
                trans_elem = ET.SubElement(msg_elem, "translation")

            if (cname, source_text) in TRANSLATIONS:
                trans_elem.text = TRANSLATIONS[(cname, source_text)]
                trans_elem.attrib.pop("type", None)
                translated_count += 1
            else:
                # Try fallback matching
                found = False
                for (c, s), val in TRANSLATIONS.items():
                    if s == source_text:
                        trans_elem.text = val
                        trans_elem.attrib.pop("type", None)
                        translated_count += 1
                        found = True
                        break
                if not found:
                    print(f"[-] Missing translation: [{cname}] {repr(source_text)}")
                    missing_count += 1

    # Inject standard Qt dialog button contexts
    for ctx_name, pairs in STANDARD_QT_STRINGS.items():
        ctx_elem = None
        for c in root.findall("context"):
            if c.find("name").text == ctx_name:
                ctx_elem = c
                break
        if ctx_elem is None:
            ctx_elem = ET.SubElement(root, "context")
            name_elem = ET.SubElement(ctx_elem, "name")
            name_elem.text = ctx_name

        existing_sources = {m.find("source").text for m in ctx_elem.findall("message") if m.find("source") is not None}
        for src, dst in pairs:
            if src not in existing_sources:
                msg = ET.SubElement(ctx_elem, "message")
                s = ET.SubElement(msg, "source")
                s.text = src
                t = ET.SubElement(msg, "translation")
                t.text = dst

    # Write formatted XML
    ET.indent(tree, space="    ")
    tree.write(TS_PATH, encoding="utf-8", xml_declaration=True)

    print(f"[+] Successfully applied Thai translations!")
    print(f"    Translated: {translated_count} messages")
    print(f"    Missing:    {missing_count} messages")

if __name__ == "__main__":
    main()

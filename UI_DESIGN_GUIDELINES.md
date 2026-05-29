# Veyon UI Design Guidelines (For AI & Developers)

Welcome, AI Coding Assistant or Developer. This document contains the mandatory design system specifications, styling guidelines, and Qt Style Sheets (QSS) blueprints for the Veyon UI modernization project. 

Whenever you are asked to modify or add any UI components, stylesheets, or visual assets in this repository, **you must strictly adhere to these specifications**.

---

## 🎨 1. Core Principles: "Light Mode First" & Sleek Aesthetics

The design system prioritizes a **Light Mode First** philosophy. It aims for a premium, clean, minimalist, and state-of-the-art interface that feels lightweight and extremely professional. Avoid generic browser-like styling, default OS behaviors, or raw primary colors (pure red, green, blue).

### The Premium Light Color Palette

| Token / Element | HEX Code | Purpose |
| :--- | :--- | :--- |
| **Primary Background** | `#f8f9fa` | Main window and default container background. |
| **Secondary Background** | `#ffffff` | Card surfaces, sidebar backgrounds, and dialog content. |
| **Hover Background** | `#f1f3f4` | List items and buttons on mouse hover. |
| **Text Primary** | `#202124` | Main labels, section titles, and active input texts (High contrast). |
| **Text Secondary** | `#5f6368` | Placeholder text, descriptions, and muted labels. |
| **Accent Primary** | `#1a73e8` | Primary active states, focused borders, selected items (Royal Blue). |
| **Accent Hover/Light** | `#e8f0fe` | Semi-transparent selection fill, checked checkbox backgrounds. |
| **Subtle Border** | `#dadce0` | Dividers, standard inputs, and card borders. |
| **Success State** | `#137333` | Approved/successful states (soft premium emerald). |
| **Warning State** | `#b06000` | Cautious actions, warnings. |

---

## 📐 2. Geometry & Soft Rounded Corners (Bo góc mềm mại)

To achieve a modern, approachable feel, all UI controls must feature soft, rounded corners instead of sharp angles.

* **Cards and Large Containers (`QGroupBox`, `QTabWidget`):** `8px` to `12px` border-radius.
* **Control Elements (`QPushButton`, `QLineEdit`, `QComboBox`, `QSpinBox`):** `6px` to `8px` border-radius.
* **Sidebar / Item Selectors (`QListWidget` items):** `6px` or `8px` border-radius.
* **Menus and Tooltips:** `6px` border-radius.

---

## 🃏 3. Card-based Structure (Cấu trúc thẻ Card)

Grouped controls must not use raw, harsh lines (`QFrame[frameShape="HLine"]`) or heavy borders. Instead, group them into clean, elegant cards:

* **Style:** Cards should have a pure white background (`#ffffff`), a thin subtle border (`1px solid #dadce0`), and padding inside to let the layout breathe.
* **Titles:** Titles for `QGroupBox` should be clean, positioned elegantly at the top-left, and colored with the **Accent Primary** (`#1a73e8`) in bold to guide the user's eye.

```css
QGroupBox {
    background-color: #ffffff;
    border: 1px solid #dadce0;
    border-radius: 10px;
    margin-top: 20px;
    padding: 20px 15px 15px 15px;
    font-weight: bold;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 15px;
    padding: 0 5px;
    color: #1a73e8;
}
```

---

## 🖋️ 4. Qt Style Sheets (QSS) Reference Blueprints

When building or updating QSS stylesheets, use the following standardized class-based styles:

### A. Sidebar / Page Selector (`QListWidget#pageSelector`)
The sidebar must look like a flat, modern, minimalist vertical navigation menu.
```css
QListWidget#pageSelector {
    background-color: #ffffff;
    border: none;
    border-right: 1px solid #dadce0;
    padding: 12px 6px;
    min-width: 180px;
}
QListWidget#pageSelector::item {
    background-color: transparent;
    color: #5f6368;
    padding: 10px 14px;
    border-radius: 8px;
    margin-bottom: 6px;
}
QListWidget#pageSelector::item:hover {
    background-color: #f1f3f4;
    color: #202124;
}
QListWidget#pageSelector::item:selected {
    background-color: #e8f0fe;
    color: #1a73e8;
    font-weight: bold;
}
```

### B. Input Controls (`QLineEdit`, `QComboBox`, `QSpinBox`)
All form inputs should appear sleek, minimal, and reactive to focus/hover states. **For numerical inputs (QSpinBox, QDoubleSpinBox), the increment/decrement spinner buttons must be completely removed.**
```css
QSpinBox, QDoubleSpinBox {
    qproperty-buttonSymbols: NoButtons; /* Hides spinner buttons */
}

QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background-color: #ffffff;
    border: 1px solid #dadce0;
    border-radius: 6px;
    padding: 6px 10px;
    color: #202124;
}
QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover {
    border: 1px solid #bdc1c6;
}
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
    border: 2px solid #1a73e8;
    padding: 5px 9px; /* Subtract 1px padding to avoid layout jump on 2px border */
}
```

### C. Push Buttons (`QPushButton`)
Buttons should feel clean, using flat-design rules with subtle hover/press shifts.
```css
QPushButton {
    background-color: #ffffff;
    border: 1px solid #dadce0;
    border-radius: 6px;
    padding: 8px 16px;
    color: #1a73e8;
    font-weight: bold;
}
QPushButton:hover {
    background-color: #f8f9fa;
    border-color: #1a73e8;
}
QPushButton:pressed {
    background-color: #f1f3f4;
}
```

---

## 📐 5. Iconography (Font Awesome Icons)

To keep the application looking sharp on modern High-DPI / Retina displays, you **must use [Font Awesome](https://fontawesome.com) Free** as the primary icon set. Replace all raster images (`.png`, `.ico`) with Font Awesome icons.

### Integration Approach

Font Awesome is integrated via **font-based rendering** in Qt for dynamic icons and **SVG files** (from Font Awesome's SVG collection) for QSS `image:` / `url()` assets (e.g., checkbox indicators).

### Font-Based Usage (C++)

1. Bundle `Font Awesome 6 Free-Solid-900.ttf` (or `Regular`, `Brands`) as a Qt resource in a `.qrc` file.
2. Load the font family once at application startup:
   ```cpp
   QFontDatabase::addApplicationFont(":/fonts/fa-solid-900.ttf");
   ```
3. Use `QIcon` with the font via a helper:

   | Icon Name | Unicode | C++ Usage |
   | :--- | :--- | :--- |
   | cog / gear | `\uf013` | `faIcon("\uf013")` |
   | power-off | `\uf011` | `faIcon("\uf011")` |
   | user | `\uf007` | `faIcon("\uf007")` |
   | search | `\uf002` | `faIcon("\uf002")` |
   | times / close | `\uf00d` | `faIcon("\uf00d")` |
   | check | `\uf00c` | `faIcon("\uf00c")` |
   | plus | `\uf067` | `faIcon("\uf067")` |
   | pen / edit | `\uf044` | `faIcon("\uf044")` |
   | trash | `\uf1f8` | `faIcon("\uf1f8")` |
   | download | `\uf019` | `faIcon("\uf019")` |

   Suggested helper pattern:
   ```cpp
   QIcon faIcon(const QString &unicode, const QColor &color = "#5f6368", int size = 16) {
       QPixmap pix(size, size);
       pix.fill(Qt::transparent);
       QPainter p(&pix);
       p.setPen(color);
       QFont font("Font Awesome 6 Free Solid");
       font.setPixelSize(size);
       p.setFont(font);
       p.drawText(QRect(0, 0, size, size), Qt::AlignCenter, unicode);
       p.end();
       return QIcon(pix);
   }
   ```

### QSS SVG Asset Integration (for indicator / decoration icons only)

When Font Awesome must appear in QSS `image:` / `url()` properties, export the individual icon as an SVG from Font Awesome's free SVG repository and bundle it:

```css
QCheckBox::indicator:checked {
    background-color: #1a73e8;
    border-color: #1a73e8;
    image: url(:/configurator/style/icons/checkbox_checked_light.svg);
}
```

### Icon Specifications:
* **Style:** Solid weight (`fa-solid`) for toolbar/action icons; Regular weight (`fa-regular`) for status/indicator icons when available.
* **Sizing:** `16px` for small indicators, `24px` to `48px` for sidebar/action toolbar icons.
* **Colors:**
  * Active/Checked state: `#ffffff` (white) overlaying the Accent Background.
  * Idle/Inactive state: `#5f6368` (slate grey).
  * Selected sidebar item: `#1a73e8` (royal blue).
* **Dark Mode:** Use `QPainter` to recolor the icon at runtime based on `VeyonCore::useDarkMode()` — no separate icon files needed.

---

## 🧱 6. Plugin Compatibility & Cascading

Because Veyon features a pluggable architecture, it is absolutely vital that:
1. **Never hardcode styles** inside specific configuration `.cpp` files. Use standard widgets, layouts, and assign distinct object names (`setObjectName`) if custom overrides are needed.
2. Rely on **QSS inheritance**. Styling base classes like `QLineEdit`, `QComboBox`, and `QGroupBox` ensures that any new or third-party plugin page loaded dynamically inherits the modern design theme automatically with zero integration code!

---

## 🖥️ 7. Veyon Master UI Specifications (Quy chuẩn riêng cho Veyon Master)

Để đảm bảo màn hình điều khiển lớp học (Master) đạt được giao diện trực quan, hiện đại giống như bản thiết kế `modern_classroom_monitor_dashboard_v1.png`, lập trình viên và AI cần áp dụng các quy chuẩn QSS và cách dựng Layout dưới đây.

### A. Phân tách khu vực & Màu nền
*   **Khu vực Sidebar trái (Rooms/Computers Tree):** Màu nền `#ffffff`. Viền phải `1px solid #dadce0`.
*   **Khu vực Dashboard Grid:** Màu nền `#f1f3f4` hoặc `#f5f6f8` để tạo độ tương phản nổi bật cho các Card máy tính.

### B. Thẻ màn hình máy tính (Computer Screen Card)
Mỗi card máy tính là một widget phức hợp bao gồm phần Screenshot/Thumbnail phía trên và một Widget Nhãn (Label Bar) phía dưới.
*   **Đổ bóng (Drop Shadow):** Phải áp dụng hiệu ứng đổ bóng cho toàn bộ Card để tạo chiều sâu bằng cách dùng `QGraphicsDropShadowEffect` trong mã nguồn C++:
    ```cpp
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 40)); // Bóng mờ nhẹ
    shadow->setOffset(0, 2);
    cardWidget->setGraphicsEffect(shadow);
    ```
*   **Bo góc QSS cho Card:**
    ```css
    /* Phần chứa thumbnail màn hình */
    QWidget#computerThumbnail {
        background-color: #202124;
        border-top-left-radius: 8px;
        border-top-right-radius: 8px;
        border: 1px solid #dadce0;
        border-bottom: none;
    }
    /* Thanh nhãn thông tin bên dưới */
    QWidget#computerLabelBar {
        background-color: #ffffff;
        border-bottom-left-radius: 8px;
        border-bottom-right-radius: 8px;
        border: 1px solid #dadce0;
        border-top: none;
        min-height: 36px;
    }
    QLabel#computerLabel {
        color: #202124;
        font-weight: bold;
        alignment: AlignCenter;
    }
    ```

### C. Menu chuột phải hiện đại (QMenu)
Giao diện `QMenu` của chuột phải trên Master cần đồng nhất, thoáng đãng và có bo góc:
```css
QMenu {
    background-color: #ffffff;
    border: 1px solid #dadce0;
    border-radius: 8px;
    padding: 6px 0px;
}
QMenu::item {
    background-color: transparent;
    color: #202124;
    padding: 8px 24px 8px 12px; /* Khoảng cách thoáng */
    margin: 2px 6px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #e8f0fe;
    color: #1a73e8;
}
QMenu::separator {
    height: 1px;
    background-color: #dadce0;
    margin: 6px 12px;
}
```

### D. Thanh công cụ dưới cùng (Bottom Bar)
```css
QWidget#bottomBar {
    background-color: #f8f9fa;
    border-top: 1px solid #dadce0;
    min-height: 50px;
}
/* Các nút chuyển đổi chế độ */
QPushButton#bottomTabButton {
    background-color: transparent;
    border: none;
    border-radius: 6px;
    padding: 6px 12px;
    color: #5f6368;
}
QPushButton#bottomTabButton:hover {
    background-color: #f1f3f4;
}
QPushButton#bottomTabButton:checked {
    background-color: #e8f0fe;
    color: #1a73e8;
    font-weight: bold;
}
```

### E. Nguyên lý Tối giản hóa Header & Loại bỏ Toolbar chính (Header-Free & Context-Driven)

Để đạt được giao diện siêu thoáng đãng như thiết kế mới, Veyon Master sẽ chuyển đổi từ mô hình "Thanh công cụ tập trung phía trên" sang mô hình "Tương tác theo ngữ cảnh chuột phải".

*   **Loại bỏ Toolbar chính (`QToolBar`):** 
    *   Hoàn toàn ẩn hoặc loại bỏ thanh `QToolBar` mặc định ở phía trên cùng của cửa sổ chính (`MainWindow`) trong veyon-master.
    *   *Kỹ thuật thực hiện trong code:* Không add toolbar vào MainWindow hoặc gọi `mainToolBar->hide()` / `removeToolBar(mainToolBar)` để giải phóng không gian phía trên.
*   **Chuyển đổi luồng tính năng:**
    *   **Tính năng điều khiển máy đơn lẻ:** Chuyển toàn bộ vào Menu chuột phải (Context Menu) của từng Computer Card (như `Lock`, `Remote view`, `Power on`, `Reboot`, v.v.).
    *   **Tính năng điều khiển/Lọc chung:** Chuyển xuống thanh điều hướng dưới cùng (**Bottom Bar**) hoặc tích hợp gọn gàng vào sidebar trái (ví dụ thanh tìm kiếm phòng học và máy tính).


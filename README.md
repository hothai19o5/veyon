# Hướng Dẫn Setup Môi Trường Build EduMonitor

Tài liệu này hướng dẫn setup môi trường build EduMonitor từ một máy Linux mới. Quy trình gồm 2 phần:

- Xây dựng môi trường build Linux và build package Linux.
- Từ môi trường Linux đó, setup MXE để cross-build EduMonitor cho Windows.

Các lệnh bên dưới giả định dùng Ubuntu/Debian 64-bit. Nếu dùng distro khác, hãy cài các package tương đương.

## 1. Xây Dựng Môi Trường Build Linux

### 1.1. Cài công cụ cơ bản

```bash
sudo apt update
sudo apt install -y \
  git build-essential cmake ninja-build pkg-config fakeroot \
  ca-certificates curl wget unzip zip xz-utils \
  gettext dos2unix
```

### 1.2. Cài thư viện build cho EduMonitor trên Linux

Build native Linux dùng Qt 6 theo mặc định của project.

```bash
sudo apt install -y \
  qt6-base-dev qt6-base-private-dev qt6-base-dev-tools \
  qt6-tools-dev qt6-tools-dev-tools qt6-5compat-dev \
  libqca-qt6-2-dev libqca-qt6-plugins \
  xorg-dev libxtst-dev libfakekey-dev \
  libjpeg-dev zlib1g-dev libpng-dev libssl-dev \
  libpam0g-dev libproc2-dev liblzo2-dev \
  libldap2-dev libsasl2-dev libvncserver-dev
```

Nếu distro của bạn chưa có `libproc2-dev`, thử thay bằng `libprocps-dev`.

### 1.3. Lấy source code

```bash
git clone --recursive -b custom-ui https://github.com/hothai19o5/veyon.git
cd veyon
```

Nếu đã clone repository nhưng thiếu submodule, chạy:

```bash
git submodule update --init --recursive
```

### 1.4. Cấu hình build Linux

Tạo thư mục build riêng để không trộn file build vào source tree.

```bash
cmake -S . -B build-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=/usr
```

Nếu CMake báo thiếu dependency, cài package tương ứng rồi chạy lại lệnh trên.

### 1.5. Build

```bash
cmake --build build-linux
```

### 1.6. Tạo package Linux

```bash
cd build-linux
fakeroot cpack
cd ..
```

Kết quả thường là file `.deb` trên Debian/Ubuntu hoặc `.rpm` trên các distro RPM.

### 1.7. Cài thử local

Không khuyến nghị cài trực tiếp lên máy production. Chỉ dùng để kiểm tra nhanh trên máy build.

```bash
sudo cmake --install build-linux
```

## 2. Setup Môi Trường Cross MXE Windows Từ Linux

Phần này dùng MXE để build toolchain MinGW-w64 và các thư viện Windows cần thiết ngay trên máy Linux đã setup ở phần 1.

Build Windows qua MXE dùng Qt 5. Không dùng Qt 6 cho phần cross-build Windows trong hướng dẫn này vì MXE hiện tại thiếu `Qca-qt6`, trong khi cấu hình Qt 5 đã build thành công.

### 2.1. Cài dependency để build MXE

```bash
sudo apt install -y \
  autoconf automake autopoint bash bison bzip2 flex g++ gperf intltool \
  libffi-dev libgdk-pixbuf-2.0-dev libltdl-dev libssl-dev \
  libtool-bin libxml-parser-perl lzip make openssl patch perl \
  python3 ruby sed texinfo unzip wget xz-utils
```

### 2.2. Lấy source MXE

Ví dụ đặt MXE ở `/opt/mxe`.

```bash
sudo git clone https://github.com/mxe/mxe.git /opt/mxe
sudo chown -R "$USER:$USER" /opt/mxe
```

### 2.3. Build toolchain và thư viện Windows

EduMonitor dùng toolchain CMake `cmake/modules/Win64Toolchain.cmake`. Toolchain này đọc biến `MXE_PATH` và mặc định target Windows 64-bit `x86_64-w64-mingw32`.

Build target shared để tạo DLL runtime cho bộ cài Windows:

```bash
cd /opt/mxe
make MXE_TARGETS='x86_64-w64-mingw32.shared' \
  gcc cmake nsis \
  qtbase qttools \
  qca openssl libjpeg-turbo libpng zlib lzo \
  libvncserver openldap cyrus-sasl
```

Quá trình này có thể mất nhiều thời gian vì MXE phải build compiler và nhiều thư viện từ source.

Nếu MXE báo không có package nào đó do thay đổi tên package, kiểm tra danh sách package hiện có bằng:

```bash
cd /opt/mxe
make show-package-list | grep -E 'qtbase|qttools|qca|vnc|ldap|sasl|nsis'
```

### 2.4. Khai báo biến môi trường MXE

```bash
export MXE_PATH=/opt/mxe
export MXE_TARGET=x86_64-w64-mingw32.shared
export PATH="$MXE_PATH/usr/bin:$PATH"
```

Nếu muốn giữ cấu hình này cho các terminal sau, thêm 3 dòng trên vào `~/.bashrc` hoặc file shell tương ứng.

### 2.5. Cấu hình cross-build Windows

Quay lại source Veyon:

```bash
cd /path/to/veyon
git submodule update --init --recursive
```

Cấu hình build Windows 64-bit bằng toolchain có sẵn trong repository. Lưu ý thêm `-DWITH_QT6=OFF` để dùng Qt 5 và `-DWITH_TRANSLATIONS=OFF` nếu chưa build/copy đầy đủ translation files:

```bash
cmake -S . -B build-win64-qt5-notrans \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/modules/Win64Toolchain.cmake \
  -DMXE_PATH="$MXE_PATH" \
  -DMINGW_TARGET="$MXE_TARGET" \
  -DCMAKE_INSTALL_PREFIX=/ \
  -DWITH_QT6=OFF \
  -DWITH_TRANSLATIONS=OFF
```

### 2.6. Build Windows binaries

```bash
nice -n 19 ionice -c3 cmake --build build-win64-qt5-notrans --parallel 1
```

Không dùng `cmake --build ... --parallel` không giới hạn job trên máy yếu vì có thể làm máy đơ/lag. Nếu máy đủ mạnh, có thể tăng lên `--parallel 2` hoặc `--parallel 4`.

### 2.7. Tạo thư mục portable Windows

Repository có target `windows-binaries` để gom file `.exe`, `.dll`, plugin, Qt runtime và tài nguyên cần thiết vào một thư mục chạy được trên Windows.

```bash
nice -n 19 ionice -c3 cmake --build build-win64-qt5-notrans --target windows-binaries --parallel 1
```

Thư mục kết quả nằm trong `build-win64-qt5-notrans` và có tên dạng:

```text
veyon-win64-x.y.z.build
```

### 2.8. Tạo bộ cài Windows bằng NSIS

Nếu MXE đã build `nsis`, tạo installer bằng target sau:

```bash
nice -n 19 ionice -c3 cmake --build build-win64-qt5-notrans --target create-windows-installer --parallel 1
```

File kết quả là bộ cài `.exe` trong thư mục `build-win64-qt5-notrans`.

## Ghi Chú Build

- Dùng `build-linux` và `build-win64-qt5-notrans` riêng biệt, không dùng chung một build directory cho Linux và Windows.
- Linux native: dùng Qt 6 mặc định.
- Windows MXE: dùng Qt 5 với `-DWITH_QT6=OFF`.
- Không dùng Qt 6 cho Windows MXE trừ khi đã tự build/cài đầy đủ `Qca-qt6` trong MXE và kiểm tra lại CMake configure thành công.
- Nếu không cần LDAP, có thể thêm `-DWITH_LDAP=OFF` để giảm dependency.
- Nếu không cần WebAPI, có thể thêm `-DWITH_WEBAPI=OFF` để giảm dependency Qt HTTP/WebSocket.
- Khi gặp lỗi thiếu DLL ở bước `windows-binaries`, kiểm tra lại MXE target phải là `x86_64-w64-mingw32.shared`, không phải static target.

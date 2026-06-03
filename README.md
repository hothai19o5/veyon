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
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCPACK_DIST=ubuntu.24.04
```

- `CPACK_DIST` là release tag ghi vào file `.deb`/`.rpm` (ví dụ `ubuntu.24.04`,
  `ubuntu.22.04`, `fedora.40`). Bắt buộc phải set, nếu không release tag trong
  package sẽ rỗng.
- Nếu chưa build/copy đầy đủ translation files (thư mục `translations/` rỗng
  hoặc thiếu `.ts`), thêm `-DWITH_TRANSLATIONS=OFF` để bỏ qua bước dịch.

Nếu CMake báo thiếu dependency, cài package tương ứng rồi chạy lại lệnh trên.

### 1.5. Build

```bash
cmake --build build-linux
```

Khi sửa source, chỉ cần chạy lại lệnh này — CMake sẽ tự detect file thay đổi
và chỉ rebuild những target liên quan.

### 1.6. Tạo package Linux

```bash
cd build-linux
fakeroot cpack -G DEB
cd ..
```

Kết quả là file `.deb` trong `build-linux/` (ví dụ `build-linux/veyon-4.10.3.2-Linux.deb`
hoặc tên tương tự tùy version).

### 1.7. Build & chạy bản dev (không cài)

Dành cho lập trình viên muốn sửa code và chạy thử nhanh mà không cần tạo
`.deb` và cài vào hệ thống.

#### 1.7.1. Cấu hình bản Debug

Nếu đã có `build-linux/` ở `RelWithDebInfo` (dùng cho release), tạo thêm
thư mục build riêng cho dev để không ảnh hưởng bản release:

```bash
cmake -S . -B build-dev -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCPACK_DIST=ubuntu.24.04
cmake --build build-dev
```

#### 1.7.2. Chạy binary từ thư mục build

Sau khi build xong, các binary nằm trong các thư mục con của `build-dev/`:

```bash
# Master (giao diện điều khiển chính)
./build-dev/master/veyon-master

# Configurator (cấu hình hệ thống)
./build-dev/configurator/veyon-configurator

# CLI (dòng lệnh)
./build-dev/cli/veyon-cli

# Server / Service / Worker (chạy nền)
./build-dev/server/veyon-server
./build-dev/service/veyon-service
./build-dev/worker/veyon-worker
```

Binary sẽ tìm plugin `.so` và resource tương đối với vị trí của nó, nên
chỉ cần chạy từ trong thư mục build là đủ — không cần `sudo` và không cần
cài đặt.

#### 1.7.3. Workflow lặp khi sửa code

```bash
# Sửa code, sau đó:
cmake --build build-dev

# Chạy lại binary đã build
./build-dev/master/veyon-master
```

Nếu thay đổi file `.ui` (Qt Designer form), CMake sẽ tự chạy lại `uic` để
sinh lại code. Không cần xóa cache trừ khi thêm/sửa option CMake hoặc thay
đổi `CMakeLists.txt` cấu trúc lớn.

#### 1.7.4. Chạy test

Nếu project có test (target `test` hoặc `unittest`):

```bash
cmake --build build-dev --target test
# hoặc
ctest --test-dir build-dev --output-on-failure
```

#### 1.7.5. Debug với gdb

```bash
gdb --args ./build-dev/master/veyon-master
```

Hoặc attach vào tiến trình đang chạy:

```bash
# Trong terminal 1: chạy app bình thường
./build-dev/master/veyon-master

# Trong terminal 2: tìm PID và attach
pgrep -f veyon-master
sudo gdb -p <PID>
```

#### 1.7.6. Mở trong Qt Creator

Qt Creator hỗ trợ CMake project trực tiếp: `File → Open File or Project…`
rồi chọn `CMakeLists.txt` ở thư mục gốc. Sau đó Qt Creator tự dò thư mục
build (`build-dev/`), cho phép sửa code, build, debug, và chạy trong cùng
một cửa sổ.

### 1.8. Cài thử local

Không khuyến nghị cài trực tiếp lên máy production. Chỉ dùng để kiểm tra
nhanh trên máy build.

File `.deb` đã tạo ở bước 1.6 nằm trong `build-linux/`. **Không dùng**
`cmake --install` trên hệ thống đã cài `.deb`, vì `cmake --install` copy
file vào `/usr/` mà dpkg không quản lý — gây xung đột khi cài hoặc gỡ
sau này.

Cài bằng dpkg (khuyến nghị):

```bash
sudo dpkg -i build-linux/veyon-*.deb
sudo apt-get -f install   # tự cài dependency nếu thiếu
```

### 1.9. Gỡ phiên bản cũ

Trước khi cài version mới, gỡ bản cũ để tránh xung đột:

```bash
# 1. Gỡ package cũ (đã cài qua .deb trước đó)
sudo apt remove --purge veyon

# 2. Nếu trước đó đã từng cài bằng `cmake --install` trên hệ thống này,
#    xóa các file sót lại bằng manifest. Chỉ chạy khi manifest vẫn còn
#    khớp với lần install gần nhất (chưa bị ghi đè bởi build mới):
sudo xargs rm -v < build-linux/install_manifest.txt

# 3. Nếu vẫn còn file ở /usr/local/ từ bản .deb cũ build với prefix lạ:
sudo rm -rf /usr/local/lib/veyon \
            /usr/local/bin/veyon-* \
            /usr/local/share/{applications,icons,pixmaps,polkit-1}/veyon* \
            /usr/local/share/polkit-1/actions/io.veyon.*

# 4. Verify đã sạch
which veyon-master veyon-configurator   # phải báo "not found"
ls /usr/bin/veyon-* /usr/lib/x86_64-linux-gnu/veyon/ \
   /usr/local/bin/veyon-* /usr/local/lib/veyon/ 2>&1   # phải trống
dpkg -l | grep veyon                    # phải trống
```

Sau đó cài bản mới theo bước 1.8.

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

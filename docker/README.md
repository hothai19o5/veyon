# Môi trường build bằng Docker

Các file Docker này bám theo quy trình trong `README.md` nhưng tách môi trường build khỏi máy host. Mục tiêu là cùng một workflow có thể chạy trên các máy có Docker mà không cần cài Qt/CMake/MXE trực tiếp lên host.

Nếu Docker trên máy yêu cầu quyền root, thêm `sudo` trước các lệnh `docker` bên dưới.

## Các target trong `Dockerfile`

- `linux-build` *(mặc định)*: Ubuntu 24.04 + Qt 6 + dependency để build native Linux và tạo `.deb`.
- `mxe-base`: Ubuntu 24.04 + dependency để build MXE. Target này **chưa build toolchain MXE** nên build image nhanh hơn.
- `mxe-full`: `mxe-base` + build sẵn MXE Qt 5/MinGW target `x86_64-w64-mingw32.shared`. Image này rất lớn và build có thể mất nhiều giờ.

Các Dockerfile trong `.ci/linux.*` vẫn dùng cho matrix CI nhiều distro. Dockerfile ở root này ưu tiên workflow local/reproducible theo README hiện tại.

## Chuẩn bị source

```bash
git submodule update --init --recursive
```

## Build Linux package `.deb`

Build image:

```bash
docker build \
  --target linux-build \
  --build-arg UID=$(id -u) \
  --build-arg GID=$(id -g) \
  -t edumonitor-build:ubuntu24.04 \
  .
```

Build source đang mount từ host. Nên dùng build dir riêng cho Docker để tránh lẫn với build dir native trên host:

```bash
docker run --rm -it \
  -v "$PWD:/workspace/veyon" \
  -w /workspace/veyon \
  edumonitor-build:ubuntu24.04 \
  env BUILD_DIR=/workspace/veyon/build-linux-docker \
  veyon-build-linux
```

Kết quả nằm trong `build-linux-docker/` trên host.

Biến môi trường hữu ích:

```bash
# Build nhưng không tạo package
docker run --rm -it -v "$PWD:/workspace/veyon" edumonitor-build:ubuntu24.04 \
  env PACKAGE=0 veyon-build-linux

# Translations mặc định đang tắt; bật lại nếu cần
docker run --rm -it -v "$PWD:/workspace/veyon" edumonitor-build:ubuntu24.04 \
  env WITH_TRANSLATIONS=ON veyon-build-linux

# Giảm số job khi máy yếu
docker run --rm -it -v "$PWD:/workspace/veyon" edumonitor-build:ubuntu24.04 \
  env BUILD_PARALLEL=1 veyon-build-linux
```

## Cross-build Windows bằng MXE

### Cách 1: build MXE vào Docker image

Cách này tạo image lớn nhưng tiện khi muốn pull image là có sẵn toolchain MXE.

```bash
docker build \
  --target mxe-full \
  --build-arg UID=$(id -u) \
  --build-arg GID=$(id -g) \
  --build-arg MXE_JOBS=1 \
  -t edumonitor-build:mxe-full \
  .
```

Sau đó build Windows:

```bash
docker run --rm -it \
  -v "$PWD:/workspace/veyon" \
  -w /workspace/veyon \
  edumonitor-build:mxe-full \
  env BUILD_DIR=/workspace/veyon/build-win64-docker \
  veyon-build-windows-mxe
```

### Cách 2: dùng named volume để cache MXE

Cách này là workflow đã dùng thành công. Image `mxe-base` nhỏ hơn `mxe-full`, còn toolchain MXE được giữ trong volume `edumonitor-mxe` để lần sau không build lại từ đầu.

```bash
docker build \
  --target mxe-base \
  --build-arg UID=$(id -u) \
  --build-arg GID=$(id -g) \
  -t edumonitor-build:mxe-base \
  .

docker volume create edumonitor-mxe

docker run --rm -it \
  -v edumonitor-mxe:/opt/mxe \
  edumonitor-build:mxe-base \
  veyon-build-mxe
```

Sau khi MXE build xong, cài Interception SDK vào volume MXE:

```bash
docker run --rm -it \
  -v edumonitor-mxe:/opt/mxe \
  edumonitor-build:mxe-base \
  veyon-install-interception-mxe
```

Kiểm tra các file Interception cần thiết:

```bash
docker run --rm -it \
  -v edumonitor-mxe:/opt/mxe \
  edumonitor-build:mxe-base \
  ls /opt/mxe/usr/x86_64-w64-mingw32.shared/bin/interception.dll \
     /opt/mxe/usr/x86_64-w64-mingw32.shared/lib/libinterception.dll.a \
     /opt/mxe/usr/x86_64-w64-mingw32.shared/include/interception.h
```

Build portable Windows directory. Dùng `build-win64-docker` để tránh lỗi CMake cache nếu trước đó đã từng configure ở path host như `/home/.../veyon`:

```bash
docker run --rm -it \
  -v "$PWD:/workspace/veyon" \
  -v edumonitor-mxe:/opt/mxe \
  -w /workspace/veyon \
  edumonitor-build:mxe-base \
  env BUILD_DIR=/workspace/veyon/build-win64-docker \
  veyon-build-windows-mxe
```

Kết quả portable nằm trong thư mục dạng:

```text
build-win64-docker/veyon-win64-<version>/
```

Ví dụ kiểm tra nhanh:

```bash
ls build-win64-docker/veyon-win64-*/veyon-master.exe
ls build-win64-docker/veyon-win64-*/platforms/qwindows.dll
ls build-win64-docker/veyon-win64-*/plugins/windows-platform.dll
```

Tạo installer NSIS:

```bash
docker run --rm -it \
  -v "$PWD:/workspace/veyon" \
  -v edumonitor-mxe:/opt/mxe \
  -w /workspace/veyon \
  edumonitor-build:mxe-base \
  env BUILD_DIR=/workspace/veyon/build-win64-docker WINDOWS_INSTALLER=1 \
  veyon-build-windows-mxe
```

## Lưu ý về Interception

Workflow Windows hiện cần các file sau trong MXE target trước khi chạy target `windows-binaries`:

```text
/opt/mxe/usr/x86_64-w64-mingw32.shared/bin/interception.dll
/opt/mxe/usr/x86_64-w64-mingw32.shared/lib/libinterception.dll.a
```

Repository chỉ có file cài driver ở `3rdparty/interception/`, không có sẵn DLL/import library. Có thể cài Interception SDK vào MXE volume bằng helper sau. Helper này tải release chính thức `oblitum/Interception`, copy `interception.h`, `interception.dll` và tạo `libinterception.dll.a` bằng `dlltool` của MXE:

```bash
docker run --rm -it \
  -v edumonitor-mxe:/opt/mxe \
  edumonitor-build:mxe-base \
  veyon-install-interception-mxe
```

Nếu đã có ZIP SDK local, mount file đó và truyền `INTERCEPTION_ZIP`:

```bash
docker run --rm -it \
  -v edumonitor-mxe:/opt/mxe \
  -v "$PWD/Interception.zip:/tmp/Interception.zip:ro" \
  edumonitor-build:mxe-base \
  env INTERCEPTION_ZIP=/tmp/Interception.zip veyon-install-interception-mxe
```

## Các biến override chính

- `SRC_DIR`: source tree, mặc định `/workspace/veyon`.
- `BUILD_DIR`: build directory (`build-linux` hoặc `build-win64-qt5-notrans`).
- `BUILD_PARALLEL`: số job build, mặc định `nproc` cho Linux và `1` cho Windows/MXE.
- `CPACK_DIST`: release tag package Linux, mặc định `ubuntu.24.04`.
- `WITH_TRANSLATIONS`: bật/tắt translations, mặc định `OFF` trong Docker.
- `WITH_LDAP`: bật/tắt LDAP khi build Windows, mặc định `OFF` nên MXE mặc định không build `openldap`/`cyrus-sasl`.
- `WITH_WEBAPI`: bật/tắt option CMake.
- `CMAKE_EXTRA_ARGS`: truyền thêm option CMake.
- `MXE_PATH`: mặc định `/opt/mxe`.
- `MXE_TARGET`: mặc định `x86_64-w64-mingw32.shared`.
- `WITH_BUILTIN_LIBVNC`: dùng LibVNC bundled trong repository khi build Windows, mặc định `ON` vì MXE hiện tại có thể không còn package `libvncserver`.
- `MXE_PACKAGES`: override danh sách package MXE cần build.

## Backup và restore MXE volume

Docker Hub không lưu Docker volume. Nếu chỉ push image `edumonitor-build:mxe-base`, máy khác vẫn cần build lại hoặc restore volume `edumonitor-mxe`.

Backup volume thành file tar:

```bash
docker run --rm \
  -v edumonitor-mxe:/opt/mxe \
  -v "$PWD:/backup" \
  ubuntu:24.04 \
  tar -C /opt -cf /backup/edumonitor-mxe.tar mxe
```

Restore trên máy khác:

```bash
docker volume create edumonitor-mxe

docker run --rm \
  -v edumonitor-mxe:/opt/mxe \
  -v "$PWD:/backup" \
  ubuntu:24.04 \
  tar -C /opt -xf /backup/edumonitor-mxe.tar
```

Nếu muốn không cần volume riêng, build và push target `mxe-full`, nhưng image sẽ lớn hơn đáng kể.

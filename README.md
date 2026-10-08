# Countdown Widget

A tiny C++/Qt 6 desktop widget for KDE Plasma (Wayland) that shows

    ## Days until <Countdown Name>

It sits on the desktop layer of your primary screen (above the wallpaper,
below your windows), has a single × button to close it, follows your active
Plasma colour scheme and font, and remembers your countdown between runs.

## Build dependencies

| Distro | Packages |
|---|---|
| Fedora | `sudo dnf install cmake gcc-c++ qt6-qtbase-devel layer-shell-qt-devel` |
| Arch / Manjaro | `sudo pacman -S cmake qt6-base layer-shell-qt` |
| openSUSE | `sudo zypper install cmake gcc-c++ qt6-base-devel layer-shell-qt6-devel` |
| Kubuntu / Debian / Neon | `sudo apt install cmake g++ qt6-base-dev liblayershellqtinterface-dev` |

`layer-shell-qt` is what pins the widget to the desktop on Wayland. Without it
the program still builds, but runs as an ordinary frameless window.

## Build and install

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    sudo cmake --install build      # installs to /usr/local/bin + app menu entry

Or skip installing and run `./build/countdown-widget` directly.

## Use

- **First run:** a dialog asks for the date (MM/DD/YYYY) and a name.
- **After that:** it opens straight to the countdown.
- **Close:** click the ×.
- **New countdown:** `countdown-widget --reset`
- On the day itself it shows "Today: <Name>!", and afterwards "## Days since <Name>".
- The count updates by itself at midnight if left running.

## Start at login

System Settings → Autostart → Add… → Add Application → *Countdown Widget*.

## Settings file

`~/.config/CountdownWidget/countdown.ini`

    [Countdown]
    date=2026-12-25
    name=Christmas

    [Position]
    corner=top-right   ; top-left, top-right, bottom-left, bottom-right
    margin=48          ; pixels from the screen edges

Edit `corner`/`margin` to move it; restart the widget to apply.

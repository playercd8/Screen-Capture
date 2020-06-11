# Screen-Capture

2003年拿書上的範例改寫的抓圖工具，2020年6月8日重新編譯小改一下。
最初來自重慶大學出版社的2001年的【VC++開發Windows程式庫】的第79個範例。

## 授權

MIT License

## 使用說明

1.使用按鍵【ScrollLock】切換視窗模式或全螢幕模式。

2.預設使用按鍵【F9】抓圖 (可以改按鍵設定)


## 目前Bug

1.視窗模式抓圖可能抓到視窗邊框外?

2.存AVI檔的部分，故障? 

3.存WMV檔的部分，只能存成320x240的? 謎? 

## 開發工具

1.Visual Studio 2019 Community edition

【使用C++的桌面開發】與【個別元件】裡的MFC相關的那幾個。

https://visualstudio.microsoft.com/zh-hant/

## 第三方Libarry

1.TonyJpegLib (它裡頭用到Independent JPEG Group的JpegLib)

https://www.codeproject.com/Articles/3603/Classes-to-read-and-write-BMP-JPEG-and-JPEG-2000

2.Independent JPEG Group的JpegLib

目前使用9d版(2020年的)

http://www.ijg.org/

3.CreateMovie

https://www.codeproject.com/Articles/5055/Create-a-movie-from-an-HBitmap

## 參考資料

1.Capturing an Image - Win32 apps | Microsoft Docs

https://docs.microsoft.com/zh-tw/windows/win32/gdi/capturing-an-image


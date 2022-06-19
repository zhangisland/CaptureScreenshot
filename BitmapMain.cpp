/**
* @Author: Zhang Siru
* @Date:  2022/06/19 15:31
* @Version: 1.0
* @Purpose: Capture and Store 1920x1080 Screenshot as .bmp file
* @Reference: MSDN Capturing An Image. https://docs.microsoft.com/en-us/windows/win32/gdi/capturing-an-image
*/

#include <Windows.h>
#include <wingdi.h>
#include <windef.h>
#include <string>
#include <chrono>
#include <ctime>
#include <iostream>
#include <omp.h>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

void CaptureImage(int idx, int mWidth=1920, int mHeight=1080);
void SaveCapturedBitmap(HWND hDesktopWnd, HDC hDesktopDC, HBITMAP hBitmap, HDC hDC, int mWidth, int mHeight, int idx);


int main(int argc, char* argv[])
{
    // 1. Set image width and height
    int width = 1920;
    int height = 1080;

    // 2. Loop function: capture screenshot
    auto millisec_start = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

#pragma omp parallel for num_threads(2)  // for improving the fps
    for (int i = 0; i < 30; ++i)
    {
        CaptureImage(i, width, height);
    }

    auto millisec_end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::cout << "----------------------time of 30 screenshots----------------------" << std::endl;
    std::cout << (millisec_end - millisec_start) << " milliseconds" << std::endl;

    system("pause"); // for .exe
    return 0;
}

/********************************************************************************
*
* @brief
* Capture and Store Screenshot as bitmap
* 
* @param[in] 
* idx: index of screenshot, 0..29
* mWidth: default 1920
* mHeight: default 1080
*
* @param[out] 
* NULL
*
********************************************************************************/
void CaptureImage(int idx, int mWidth, int mHeight) {
    // 1. Acquire the Full Screen window handle using the function GetDesktopWindow();
    HWND hDesktopWnd = GetDesktopWindow();

    // 2. Get the DC(device context) of the desktop window using the function GetDC();
    HDC hDesktopDC = GetDC(hDesktopWnd);

    // 3. Create a compatible DC for the Desktop DC and a compatible bitmap to select into that compatible DC
    // Using function CreateCompatibleDC() and CreateCompatibleBitmap() to do above;
    // Using function SelectObject() to select the bitmap into our DC;
    HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);
    HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, mWidth, mHeight);  // HBITMAP is the handle of bitmap, 1920x1080
    SelectObject(hCaptureDC, hCaptureBitmap);

    // 4. Blit the contents of the Desktop DC into the created compatible DC
    // The compatible bitmap we created now contains the contents of the screen at the moment of the capture.
    BitBlt(hCaptureDC, 0, 0, mWidth, mHeight, hDesktopDC, 0, 0, SRCCOPY | CAPTUREBLT);

    // 5. Save the bitmap
    SaveCapturedBitmap(hDesktopWnd, hDesktopDC,  hCaptureBitmap, hCaptureDC, mWidth, mHeight, idx); //here to save the captured image to disk

    // 6. Release the objects.
    ReleaseDC(hDesktopWnd, hDesktopDC);
    DeleteDC(hCaptureDC);
    DeleteObject(hCaptureBitmap);
}

/*********************************************************************************
*
* @brief
* Save the bitmap into file, i.e. mouseX_mouseY_TIMESTAMP.bmp
*
* @param[in]
* hDesktopWnd: DesktopWindow handle
* hDesktopDC: DesktopWindow DC handle
* hBitmap: Bitmap handle
* hDC: CapturedDC handle
* mWidth: width of screenshot
* mHeight: height of screenshot
* idx: index of screenshot, 0..29
*
* @param[out]
* NULL
*
********************************************************************************/
void SaveCapturedBitmap(HWND hDesktopWnd, HDC hDesktopDC, HBITMAP hBitmap, HDC hDC, int mWidth, int mHeight, int idx) {
    //1.  init bitmap info header
    BITMAP bitmap;
    BITMAPINFOHEADER infoHdr; 
    
    // 2. Get the BITMAP from the HBITMAP
    // retrieve the bitmap color format, width, and height
    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bitmap);
    infoHdr.biSize = sizeof(BITMAPINFOHEADER);
    infoHdr.biWidth = bitmap.bmWidth;
    infoHdr.biHeight = bitmap.bmHeight;
    infoHdr.biPlanes = 1;
    infoHdr.biBitCount = 24;
    infoHdr.biCompression = BI_RGB;
    infoHdr.biSizeImage = 0;
    infoHdr.biXPelsPerMeter = 0;
    infoHdr.biYPelsPerMeter = 0;
    infoHdr.biClrUsed = 0;
    infoHdr.biClrImportant = 0;

    DWORD dwBmpSize = 0;
    dwBmpSize = ((bitmap.bmWidth * infoHdr.biBitCount + 31) / 32) * 4 * bitmap.bmHeight;  // what if change 32 to 24 and change 4 to 3, seems like no difference

    HANDLE hDIB = NULL;
    char* lpBitmap = NULL;
    hDIB = GlobalAlloc(GHND, dwBmpSize);
    lpBitmap = (char*)GlobalLock(hDIB);


    // 3. get the coordinates of mouse
    POINT mousePt;
    GetCursorPos(&mousePt);

    // 4. Get timestamp

    auto tnow = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    
    // 5. Gets the "bits" from the bitmap, and copies them into a buffer that's pointed to by lpbitmap.
    GetDIBits(hDesktopDC, hBitmap, 0,
        (UINT)bitmap.bmHeight,
        lpBitmap,
        (BITMAPINFO*)&infoHdr, DIB_RGB_COLORS);

    HANDLE hFile = NULL;
    // 6. A file is created, this is where we will save the screen capture.
    std::string str_filename = std::to_string(idx) + "+" + std::to_string(mousePt.x) + "_" + std::to_string(mousePt.y) + "_" + std::to_string(tnow) + ".bmp";
    LPCSTR filename = str_filename.c_str();

    hFile = CreateFileA(filename,
        GENERIC_WRITE | GENERIC_READ,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    // 7. Add the size of the headers to the size of the bitmap to get the total file size.
    DWORD dwSizeofDIB = 0;
    dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // 8. Offset to where the actual bitmap bits start.
    BITMAPFILEHEADER bmpFileHdr;
    bmpFileHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    // 9. Size of the file.
    bmpFileHdr.bfSize = dwSizeofDIB;

    // 10. bfType must always be BM for Bitmaps.
    bmpFileHdr.bfType = 0x4D42; // BM.

    DWORD dwBytesWritten = 0;

    // 11. WriteFile
    WriteFile(hFile, (LPSTR)&bmpFileHdr, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&infoHdr, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpBitmap, dwBmpSize, &dwBytesWritten, NULL);

    // 12. Unlock and Free the DIB from the heap.
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);

    // 13. Close the handle for the file that was created.
    CloseHandle(hFile);
}
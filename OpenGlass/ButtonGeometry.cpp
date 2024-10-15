#include "pch.h"
#include "ButtonGeometry.hpp"
#include "Shared.hpp"

using namespace OpenGlass::Shared;
using namespace OpenGlass;
namespace OpenGlass::ButtonGeometry
{
	/*static SIZE g_xBtnSize = {49, 20};
	static SIZE g_xBtnLoneSize = { 49, 20 };
	static SIZE g_xBtnPaletteSize = { 17, 17 };
	static SIZE g_midBtnSize = { 27, 20 };
	static SIZE g_edgeBtnSize = { 29, 20 };*/

    float xBtnHeight = 20;
    float xBtnHeightLone = 20;
    float xBtnHeightPalette = 17;
    float midBtnHeight = 20;
    float edgeBtnHeight = 20;

    float xBtnWidth = 49;
    float xBtnWidthLone = 49;
    float xBtnWidthPalette = 17;
    float midBtnWidth = 27;
    float edgeBtnWidth = 29;

	static float g_targetTBHeight = 21;
	static float g_targetTBHeightPalette = 17;

	static int g_xBtnInsetTopNormal = 1;
	static int g_xBtnInsetTopLoneNormal = 1;
	static int g_midBtnInsetTopNormal = 1;
	static int g_edgeBtnInsetTopNormal = 1;

	static int g_xBtnInsetDirectionalPalette = 0;

	static int g_xBtnInsetTopMaximized = -1;
	static int g_xBtnInsetTopLoneMaximized = -1;
	static int g_midBtnInsetTopMaximized = -1;
	static int g_edgeBtnInsetTopMaximized = -1;
	// dulappy: ADD EXTRA OPTIONS HERE FOR HOW MANY TIMES THE BORDER WILL BE TAKEN INTO ACCOUNT (LIKE FOR THE ICON)

	static int g_xBtnOffsetNormal = -2;
	static int g_xBtnOffsetMaximized = 2;
	static int g_xBtnOffsetPalette = 1;

	static int g_xBtnInsetAfter = 0;
	static int g_midBtnInsetAfter = 0;
	static int g_edgeBtnInsetAfter = 0;  // dulappy: doesn't really do much apart from making the space for the text smaller

	static int g_insetLeftAddNormal = 0;       // a static number added to the inset
	static int g_insetLeftMulNormal = 1;       // how many times the border size will be taken into account
	static int g_insetLeftAddMaximized = 0;
	static int g_insetLeftMulMaximized = 1;

    static float g_buttonOpacity = 1.0f;

    HRESULT STDMETHODCALLTYPE CButton_SetVisualStates(void* This, uDwm::CBitmapSourceArray* array1, uDwm::CBitmapSourceArray* array2, uDwm::CBitmapSource* bmpsrc, float opacity);
    decltype(&CButton_SetVisualStates) g_CButton_SetVisualStates_Orig{ nullptr };

	long STDMETHODCALLTYPE CTLW_UpdateNCAreaPositionsAndSizes(uDwm::CTopLevelWindow* This);
	decltype(&CTLW_UpdateNCAreaPositionsAndSizes) g_CTLW_UpdateNCAreaPositionsAndSizes_Orig{ nullptr };

	void SetInsetFromParentLeft(uDwm::CVisual* This, int inset) {
		if (*(DWORD*)((BYTE*)This + 128) != inset) {
			*(DWORD*)((BYTE*)This + 128) = inset;
			This->SetDirtyFlags(2);
		}
	}
	void SetInsetFromParentRight(uDwm::CVisual* This, int inset) {
		if (*(DWORD*)((BYTE*)This + 132) != inset) {
			*(DWORD*)((BYTE*)This + 132) = inset;
			This->SetDirtyFlags(2);
		}
	}
	void SetInsetFromParentTop(uDwm::CVisual* This, int inset) {
		if (*(DWORD*)((BYTE*)This + 136) != inset) {
			*(DWORD*)((BYTE*)This + 136) = inset;
			This->SetDirtyFlags(2);
		}
	}
	void SetInsetFromParentBottom(uDwm::CVisual* This, int inset) {
		if (*(DWORD*)((BYTE*)This + 140) != inset) {
			*(DWORD*)((BYTE*)This + 140) = inset;
			This->SetDirtyFlags(2);
		}
	}
	void SetInsetFromParent(uDwm::CVisual* This, MARGINS* inset) {
		MARGINS* insetFromParent = (MARGINS*)((BYTE*)This + 128);
		if (insetFromParent->cxLeftWidth != inset->cxLeftWidth
			|| insetFromParent->cxRightWidth != inset->cxRightWidth
			|| insetFromParent->cyTopHeight != inset->cyTopHeight
			|| insetFromParent->cyBottomHeight != inset->cyBottomHeight)
		{
			*(MARGINS*)((BYTE*)This + 128) = *inset;
			This->SetDirtyFlags(2);
		}
	}

	long CTopLevelWindow_UpdateNCAreaButton(BYTE* pThis, int buttonId, int insetTop, int* insetRight, int DPIValue) {
        if (!*(long long*)(pThis + 488 + 8 * buttonId)) { // if this button doesn't exist on the window, return.
            return 0;
        }
        uDwm::CVisual* buttonVisual = *(uDwm::CVisual**)(pThis + 488 + 8 * buttonId);
        uDwm::CVisual* windowVisual = *(uDwm::CVisual**)(pThis + 728);
        MARGINS borderMargins = *(MARGINS*)(pThis + 612);
        bool isMaximized = *(pThis + 240) & 0x04;
        bool isTool = *(pThis + 592) & 2;

        int TBHeight;
        int width;
        int height;
        if (isTool) {
            TBHeight = GetSystemMetricsForDpi((int)53, (UINT)DPIValue);
        }
        else {
            TBHeight = GetSystemMetricsForDpi((int)31, (UINT)DPIValue);
        }

        int fullTBHeight = TBHeight + *(DWORD*)((BYTE*)windowVisual + 96) + 1;

        SetInsetFromParentRight(buttonVisual, *insetRight);

        if (isTool) {
            height = floor(TBHeight * (double)(xBtnHeightPalette / g_targetTBHeightPalette) + 0.5);
            width = floor(TBHeight * (double)(xBtnWidthPalette / g_targetTBHeightPalette) + 0.5);

            if (g_tbAlignPalette == TBAlign::Full) {
                BYTE* windowVisual = *(BYTE**)(pThis + 96);
                height = fullTBHeight;
            }
            *insetRight += width;
        }
        else {
            if (buttonId == 3) {
                if ((*(DWORD*)(pThis + 592) & 0xB00) == 0) {
                    width = floor(TBHeight * (double)(xBtnWidthLone / g_targetTBHeight) + 0.5);
                    height = floor(TBHeight * (double)(xBtnHeightLone / g_targetTBHeight) + 0.5);

                    int insetAdd;

                    if (isMaximized) {
                        insetAdd = g_xBtnInsetTopLoneMaximized;
                    }
                    else {
                        insetAdd = g_xBtnInsetTopLoneNormal;
                    }

                    switch (g_tbAlign) {
                        case TBAlign::Top:
                        case TBAlign::Full:
                            insetTop += insetAdd;
                            break;
                        case TBAlign::Center:
                            insetTop += (fullTBHeight - height - insetTop) / 2;
                            break;
                        case TBAlign::TBCenter:
                            insetTop = fullTBHeight - TBHeight - 1;
                            insetTop += (TBHeight - height) / 2;
                            break;
                        case TBAlign::Bottom:
                            insetTop = fullTBHeight - height - insetAdd;
                            break;
                    }
                }
                else {
                    width = floor(TBHeight * (double)(xBtnWidth / g_targetTBHeight) + 0.5);
                    height = floor(TBHeight * (double)(xBtnHeight / g_targetTBHeight) + 0.5);

                    int insetAdd;

                    if (isMaximized) {
                        insetAdd = g_xBtnInsetTopMaximized;
                    }
                    else {
                        insetAdd = g_xBtnInsetTopNormal;
                    }

                    switch (g_tbAlign) {
                        case TBAlign::Top:
                        case TBAlign::Full:
                            insetTop += insetAdd;
                            break;
                        case TBAlign::Center:
                            insetTop += (fullTBHeight - height - insetTop) / 2;
                            break;
                        case TBAlign::TBCenter:
                            insetTop = fullTBHeight - TBHeight - 1;
                            insetTop += (TBHeight - height) / 2;
                            break;
                        case TBAlign::Bottom:
                            insetTop = fullTBHeight - height - insetAdd;
                            break;
                    }
                }
                *insetRight += width + g_xBtnInsetAfter;
            }
            else if ((buttonId != 1 || *(long long*)(pThis + 488)) && buttonId) {
                width = floor(TBHeight * (double)(midBtnWidth / g_targetTBHeight) + 0.5);
                height = floor(TBHeight * (double)(midBtnHeight / g_targetTBHeight) + 0.5);

                int insetAdd;

                if (isMaximized) {
                    insetAdd = g_midBtnInsetTopMaximized;
                }
                else {
                    insetAdd = g_midBtnInsetTopNormal;
                }

                switch (g_tbAlign) {
                    case TBAlign::Top:
                    case TBAlign::Full:
                        insetTop += insetAdd;
                        break;
                    case TBAlign::Center:
                        insetTop += (fullTBHeight - height - insetTop) / 2;
                        break;
                    case TBAlign::TBCenter:
                        insetTop = fullTBHeight - TBHeight - 1;
                        insetTop += (TBHeight - height) / 2;
                        break;
                    case TBAlign::Bottom:
                        insetTop = fullTBHeight - height - insetAdd;
                        break;
                }

                *insetRight += width + g_midBtnInsetAfter;
            }
            else {
                width = floor(TBHeight * (double)(edgeBtnWidth / g_targetTBHeight) + 0.5);
                height = floor(TBHeight * (double)(edgeBtnHeight / g_targetTBHeight) + 0.5);

                int insetAdd;

                if (isMaximized) {
                    insetAdd = g_edgeBtnInsetTopMaximized;
                }
                else {
                    insetAdd = g_edgeBtnInsetTopNormal;
                }

                switch (g_tbAlign) {
                    case TBAlign::Top:
                    case TBAlign::Full:
                        insetTop += insetAdd;
                        break;
                    case TBAlign::Center:
                        insetTop += (fullTBHeight - height - insetTop) / 2;
                        break;
                    case TBAlign::TBCenter:
                        insetTop = fullTBHeight - TBHeight;
                        insetTop += (TBHeight - height - 0.5f) / 2;
                        break;
                    case TBAlign::Bottom:
                        insetTop = fullTBHeight - height - insetAdd;
                        break;
                }

                *insetRight += width + g_edgeBtnInsetAfter;
            }

            // This could be reworked to use fullTBHeight instead, at the cost of losing Windows 10 parity.
            if (g_tbAlign == TBAlign::Full) {
                height = TBHeight + *(DWORD*)(windowVisual + 96);
                if (height > borderMargins.cyTopHeight - insetTop) {
                    height -= insetTop;
                }
            }
        }
        SetInsetFromParentTop(buttonVisual, insetTop);

        SIZE size = { width, height };
        buttonVisual->SetSize(&size);
        return 0;
	}
}

HRESULT STDMETHODCALLTYPE ButtonGeometry::CButton_SetVisualStates(void* This, uDwm::CBitmapSourceArray* array1, uDwm::CBitmapSourceArray* array2, uDwm::CBitmapSource* bmpsrc, float opacity)
{
    if (opacity != 1.f)
        opacity = g_buttonOpacity;
    return g_CButton_SetVisualStates_Orig(This, array1, array2, bmpsrc, opacity);
}

long STDMETHODCALLTYPE ButtonGeometry::CTLW_UpdateNCAreaPositionsAndSizes(
	uDwm::CTopLevelWindow* This
)
{
	BYTE* pThis = (BYTE*)This;
	MARGINS clientMargins = *(MARGINS*)(pThis + 596);

	bool isMaximized = *(pThis + 240) & 0x04;
	bool isTool = *(pThis + 592) & 2;

	uDwm::CVisual* windowVisual = *(uDwm::CVisual**)(pThis + 728);
	int DPIValue = *(int*)((BYTE*)windowVisual + 324);

	if (*(long long*)(pThis + 480)) {
		uDwm::CVisual* clientVisual = *(uDwm::CVisual**)(pThis + 544);
		SetInsetFromParentLeft(clientVisual, clientMargins.cxLeftWidth);
		SetInsetFromParentRight(clientVisual, clientMargins.cxRightWidth);
		SetInsetFromParentTop(clientVisual, clientMargins.cyTopHeight);
		SetInsetFromParentBottom(clientVisual, clientMargins.cyBottomHeight);

		// seems unused, included here anyway just in case
		BYTE* clientBlur = *(BYTE**)(pThis + 296);
		if (clientBlur) {
			SetInsetFromParent(clientVisual, (MARGINS*)((BYTE*)clientVisual + 128));
		}
	}

	MARGINS* borderSizes;
	if (isMaximized) {
		borderSizes = (MARGINS*)(pThis + 644);
	}
	else {
		borderSizes = (MARGINS*)(pThis + 628);
	}

	// left inset calculations for icon and text
	int insetLeft = g_insetLeftAddNormal + g_insetLeftMulNormal * clientMargins.cxLeftWidth;
	if (isMaximized) {
		insetLeft = g_insetLeftAddMaximized + g_insetLeftMulMaximized * clientMargins.cxLeftWidth;
	}

	// right inset calculations for buttons
	// might add some more options here later
	int insetRight = clientMargins.cxRightWidth;
	if (insetRight <= 0) {
		insetRight = *(int*)((BYTE*)windowVisual + 96);
	}

	if (isMaximized) {
		insetRight = borderSizes->cxRightWidth + g_xBtnOffsetMaximized;
	}
	else {
		insetRight += g_xBtnOffsetNormal;
	}

	int insetTop = borderSizes->cyTopHeight;

	if (isTool) {
		int TBHeight = GetSystemMetricsForDpi(53, DPIValue);
		int fullTBHeight = TBHeight + *(DWORD*)((BYTE*)windowVisual + 96) + 1;
		int btnHeight = floor(TBHeight * (double)(xBtnHeightPalette / g_targetTBHeightPalette) + 0.5);

		switch (g_tbAlignPalette) {
			case TBAlign::Top:
				insetTop += g_xBtnInsetDirectionalPalette;
				break;
			case TBAlign::Center:
				insetTop += (fullTBHeight - btnHeight - insetTop) / 2;
				break;
			case TBAlign::TBCenter:
				insetTop = fullTBHeight - TBHeight - 1;
				insetTop += (TBHeight - btnHeight) / 2;
				break;
			case TBAlign::Bottom:
				insetTop = fullTBHeight - btnHeight - g_xBtnInsetDirectionalPalette;
				break;
		}
		insetRight += g_xBtnOffsetPalette;
	}

	CTopLevelWindow_UpdateNCAreaButton(pThis, 3, insetTop, &insetRight, DPIValue);
	CTopLevelWindow_UpdateNCAreaButton(pThis, 2, insetTop, &insetRight, DPIValue);
	CTopLevelWindow_UpdateNCAreaButton(pThis, 1, insetTop, &insetRight, DPIValue);
	CTopLevelWindow_UpdateNCAreaButton(pThis, 0, insetTop, &insetRight, DPIValue);

    uDwm::CVisual* iconVisual = *(uDwm::CVisual**)(pThis + 528);
    if (iconVisual) {
        tagSIZE size = { 0 };
        if (*(int*)((BYTE*)windowVisual + 136) || *(int*)((BYTE*)windowVisual + 140) || (*(DWORD*)(pThis + 592) & 0x10000) == 0) {
            size.cx = GetSystemMetricsForDpi(SM_CXSMICON, DPIValue);
            size.cy = GetSystemMetricsForDpi(SM_CYSMICON, DPIValue);
        }
        iconVisual->SetSize(&size);

        if (g_iconTextVerticalAlign == TxtVertAlign::Center) {
            insetTop = borderSizes->cyTopHeight + (clientMargins.cyTopHeight - size.cx - borderSizes->cyTopHeight) / 2;
        }
        else {
            int borderSize = *(DWORD*)((BYTE*)windowVisual + 96);
            insetTop = borderSize + (clientMargins.cyTopHeight - size.cx - borderSize - 1) / 2;
        }
        SetInsetFromParentLeft(iconVisual, insetLeft);
        SetInsetFromParentTop(iconVisual, insetTop);

        if (size.cx > 0) {
            insetLeft += size.cx + g_textInset;
        }
    }

    uDwm::CVisual* textVisual = *(uDwm::CVisual**)(pThis + 520);
    if (textVisual) {
        int borderSize;
        if (g_iconTextVerticalAlign == TxtVertAlign::Center)
            borderSize = borderSizes->cyTopHeight;
        else
            borderSize = *(DWORD*)((BYTE*)windowVisual + 96);

        tagSIZE size = { 0, clientMargins.cyTopHeight - borderSize - 2 };

        SetInsetFromParentTop(textVisual, borderSize + 1);
        //CVisual_SetInsetFromParentLeft(textVisual, insetLeft);

        // Changed from above to support my hacky method of handling text glow
        SetInsetFromParentLeft(textVisual, 1 + borderSizes->cxLeftWidth);
        SetInsetFromParentRight(textVisual, 1);
        textVisual->SetSize(&size);

        /*// place this somewhere it will update on window resize.
        TEXTEX* textex = *(TEXTEX**)((BYTE*)textVisual + 400);
        //RECT winrc = *(RECT*)(windowVisual + CWD_WindowRect);
        //textex->tbWidth = winrc.right - winrc.left;
        textex->textInset = { 0 };
        textex->textInset.cxLeftWidth = insetLeft - 1 - borderSizes->cxLeftWidth;
        textex->textInset.cxRightWidth = insetRight - 1;*/


        // SOME TESTING CODE. IT CAN BE SAFELY REMOVED AND IS NOT SUPPOSED TO BE HERE.

        /*RECT rc = {winrc.left + borderSizes->cxLeftWidth,
            winrc.top + borderSizes->cyTopHeight,
            winrc.right + borderSizes->cxRightWidth,
            winrc.bottom + borderSizes->cyBottomHeight,
        };

        float DPI = *(float*)(windowVisual + CWD_DPIFloat);
        *(float*)(windowVisual + CWD_DPIFloat) = 1.f;
        fprintf(stream, "%f\n", DPI);
        fprintf(stream, "%i\n", *(int*)(windowVisual + CWD_WindowRectRight));
        HRGN rgn = CreateRectRgnIndirect(&winrc);
        SetWindowRgnEx(*(HWND*)(windowVisual + CWD_HWND), rgn, 1);
        DeleteObject(rgn);*/
    }

    This->UpdatePinnedParts();
    return 0;
}

void ButtonGeometry::UpdateConfiguration(ConfigurationFramework::UpdateType type)
{

}

HRESULT ButtonGeometry::Startup()
{
    uDwm::GetAddressFromSymbolMap("CButton::SetVisualStates", g_CButton_SetVisualStates_Orig);
	uDwm::GetAddressFromSymbolMap("CTopLevelWindow::UpdateNCAreaPositionsAndSizes", g_CTLW_UpdateNCAreaPositionsAndSizes_Orig);
	return HookHelper::Detours::Write([]()
	{
		HookHelper::Detours::Attach(&g_CTLW_UpdateNCAreaPositionsAndSizes_Orig, CTLW_UpdateNCAreaPositionsAndSizes);
        HookHelper::Detours::Attach(&g_CButton_SetVisualStates_Orig, CButton_SetVisualStates);
	});
}

void ButtonGeometry::Shutdown()
{
	HookHelper::Detours::Write([]()
	{
		HookHelper::Detours::Detach(&g_CTLW_UpdateNCAreaPositionsAndSizes_Orig, CTLW_UpdateNCAreaPositionsAndSizes);
        HookHelper::Detours::Detach(&g_CButton_SetVisualStates_Orig, CButton_SetVisualStates);
	});
}
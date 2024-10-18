#include "pch.h"
#include "HookHelper.hpp"
#include "OSHelper.hpp"
#include "uDwmProjection.hpp"
#include "CaptionTextHandler.hpp"
#include "CustomMsstyleLoader.hpp"
#include "MiosD2D1.hpp"
#include "MiosUxThemeHelper.hpp"
#include "Shared.hpp"
using namespace OpenGlass::Shared;

using namespace OpenGlass;
namespace OpenGlass::CaptionTextHandler
{
    void STDMETHODCALLTYPE MyCText_SetColor(uDwm::CText* This, COLORREF color);
    void STDMETHODCALLTYPE MyCText_SetBackgroundColor(uDwm::CText* This, COLORREF color);
	long STDMETHODCALLTYPE MyCText_SetText(uDwm::CText* This, wchar_t* str);
	void STDMETHODCALLTYPE MyCText_SetFont(uDwm::CText* This, LOGFONTW* font);
    HRESULT STDMETHODCALLTYPE MyCText_InitializeVisualTreeClone(uDwm::CText* This, uDwm::CText* pNew, UINT options);
	HRESULT STDMETHODCALLTYPE MyCText_ValidateResources(uDwm::CText* This);
	uDwm::CText* STDMETHODCALLTYPE MyCText_CText(uDwm::CText* This);
	uDwm::CText* STDMETHODCALLTYPE MyCText_Destroy(uDwm::CText* This, UINT someFlags);

    long STDMETHODCALLTYPE MyCDesktopManager_LoadTheme(uDwm::CDesktopManager* This);
    void STDMETHODCALLTYPE MyCDesktopManager_UnloadTheme(uDwm::CDesktopManager* This);

    HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_UpdateWindowVisuals(uDwm::CTopLevelWindow* This);
    decltype(&MyCTopLevelWindow_UpdateWindowVisuals) g_CTopLevelWindow_UpdateWindowVisuals_Org{ nullptr };

    decltype(&MyCDesktopManager_LoadTheme) g_CDesktopManager_LoadTheme_Org{ nullptr };
    decltype(&MyCDesktopManager_UnloadTheme) g_CDesktopManager_UnloadTheme_Org{ nullptr };

    decltype(&MyCText_InitializeVisualTreeClone) g_CText_InitializeVisualTreeClone_Org{ nullptr };
	decltype(&MyCText_ValidateResources) g_CText_ValidateResources_Org{ nullptr };
	decltype(&MyCText_CText) g_CText_CText_Org{ nullptr };
	decltype(&MyCText_Destroy) g_CText_Destroy_Org{ nullptr };
	decltype(&MyCText_SetText) g_CText_SetText_Org{ nullptr };
	decltype(&MyCText_SetFont) g_CText_SetFont_Org{ nullptr };
    decltype(&MyCText_SetColor) g_CText_SetColor_Org{ nullptr };
    decltype(&MyCText_SetBackgroundColor) g_CText_SetBackgroundColor_Org{ nullptr };

    IWICBitmap* DWMAtlas{};
	uDwm::CText* g_textVisual{ nullptr };
	LONG g_textWidth{ 0 };
	LONG g_textHeight{ 0 };
	std::optional<COLORREF> g_captionColor{};

	constexpr int standardGlowSize{ 15 };
	int g_textGlowSize{ standardGlowSize };
	bool g_centerCaption{ false };
	bool g_disableTextHooks{ false };

    ID2D1Factory* d2dFactory;
    IDWriteFactory* dwriteFactory;

    HRESULT AWM_GetDWMWindowAtlas() {
        __int64* CDM_pDesktopManagerInstance = (__int64*)uDwm::CDesktopManager::s_pDesktopManagerInstance;
        HRESULT hr = 0;
        IWICBitmapSource* bmpsource;
        //IWICImagingFactory* imagingfactory = *(IWICImagingFactory**)(*CDM_pDesktopManagerInstance + 312);
        IWICImagingFactory2* imagingfactory = uDwm::CDesktopManager::s_pDesktopManagerInstance->GetWICFactory();

        HTHEME hTheme = *(HTHEME*)(CDM_pDesktopManagerInstance + 608);
        HINSTANCE hInstance = *(HINSTANCE*)(CDM_pDesktopManagerInstance + 616);
        hr = GetMiosThemeAtlas(hTheme, hInstance, imagingfactory, &bmpsource);

        hr = imagingfactory->CreateBitmapFromSource(
            bmpsource,
            WICBitmapCacheOnDemand,
            &DWMAtlas
        );

        if (bmpsource)
            bmpsource->Release();

        return hr;
    }

    void AWM_DestroyDWMWindowAtlas() {
        if (DWMAtlas)
            DWMAtlas->Release();
    }

	void CText_CreateTextLayout(uDwm::CText* This) {
        BYTE* pThis = (BYTE*)This;
		HRESULT hr = 0;
		LPCWSTR string = *(LPCWSTR*)(pThis + 288);
        TEXTEX* textex = This->GetTextEx();
		IDWriteTextLayout* textlayout = NULL;
		IDWriteTextFormat* textformat = textex->textFormat;
		LOGFONT font = *(LOGFONT*)(pThis + 296);
		if (string) {
			int stringlength = wcslen(string);
			DWRITE_TEXT_RANGE range = { 0, stringlength };
			hr = dwriteFactory->CreateTextLayout(string, stringlength, textformat, 0, 0, &textlayout);
			if (hr < 0)
				goto release;
			hr = textlayout->SetStrikethrough(font.lfStrikeOut, range);
			if (hr < 0)
				goto release;
			hr = textlayout->SetUnderline(font.lfUnderline, range);
			if (hr < 0)
				goto release;
		}
	release:
		if (hr >= 0) {
			if (textex->textLayout)
				textex->textLayout->Release();
			textex->textLayout = textlayout;
		}
	}

	void CText_CreateTextFormat(uDwm::CText* This, LOGFONTW* font) {
        BYTE* pThis = (BYTE*)This;
		HRESULT hr = 0;
		IDWriteTextFormat* textformat;
		IDWriteInlineObject* trimmingsign;
		DWRITE_TRIMMING trimming;
        TEXTEX* textex = This->GetTextEx();

		DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
		if (font->lfItalic)
			style = DWRITE_FONT_STYLE_ITALIC;
		float height = (float)-font->lfHeight;
		if (height < 0) {
			height = -height;
		}
		else if (height == 0) {
			height = -(float)((*(LOGFONT*)((__int64*)uDwm::CDesktopManager::s_pDesktopManagerInstance + 352)).lfHeight);
		}
		hr = dwriteFactory->CreateTextFormat(
			font->lfFaceName,
			NULL,
			(DWRITE_FONT_WEIGHT)font->lfWeight,
			style,
			DWRITE_FONT_STRETCH_NORMAL,
			height,
			L"en-us",
			&textformat
		);
		if (hr < 0)
			goto release;

		textformat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		hr = dwriteFactory->CreateEllipsisTrimmingSign(textformat, &trimmingsign);
		if (hr < 0)
			goto release;
		trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, NULL, 0 };
		hr = textformat->SetTrimming(&trimming, trimmingsign);
		if (hr < 0)
			goto release;
		/*if (CVisualFlags & CVIS_FLAG_RTL) {
			hr = textformat->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
			if (hr < 0)
				goto release;
		}

		if (awmsettings.textAlignment == AWM_TEXT_CENTER_ICONBUTTONS) {
			textformat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		}
		else if (awmsettings.textAlignment == AWM_TEXT_RIGHT) {
			textformat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
		}*/
	release:
		if (hr >= 0) {
			if (textex->textFormat)
				textex->textFormat->Release();
			textex->textFormat = textformat;
			CText_CreateTextLayout(This);
		}
	}
}
long STDMETHODCALLTYPE CaptionTextHandler::MyCDesktopManager_LoadTheme(uDwm::CDesktopManager* This)
{
    long rv = g_CDesktopManager_LoadTheme_Org(This);
    //rv = AWM_GetDWMWindowAtlas();

    return rv;
}
void STDMETHODCALLTYPE CaptionTextHandler::MyCDesktopManager_UnloadTheme(uDwm::CDesktopManager* This)
{
    g_CDesktopManager_UnloadTheme_Org(This);
    //AWM_DestroyDWMWindowAtlas();
}
HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCText_InitializeVisualTreeClone(uDwm::CText* This, uDwm::CText* pNew, UINT options)
{
    // The pointer to the TEXTEX struct is saved, as the function overwrites the pointer.
    BYTE* pThis = (BYTE*)This;
    int rv = g_CText_InitializeVisualTreeClone_Org(This, pNew, options);

    if (*(BYTE*)(pThis + 411) == false) {
        This->SetTextEx();
        TEXTEX* textex = This->GetTextEx();
        *(pThis + 411) = true;
        textex->render = true;
    }
    pNew->SetTextEx();
    TEXTEX* textex1 = This->GetTextEx();
    TEXTEX* textex2 = pNew->GetTextEx();
    textex2->color = textex1->color;
    textex2->shadowcolor = textex1->shadowcolor;
    textex2->tbWidth = textex1->tbWidth;
    textex2->glowopacity = textex1->glowopacity;
    textex2->textInset = textex1->textInset;
    //CopyMemory(textex2, textex1, sizeof(TEXTEX));
    //textex2->textFormat = textex1->textFormat;
    //textex2->textLayout = NULL;
    //textex2->render = true;

    return rv;
}
HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCText_ValidateResources(uDwm::CText* This)
{
    BYTE* pThis = (BYTE*)This;
    __int64* CDM_pDesktopManagerInstance = (__int64*)uDwm::CDesktopManager::s_pDesktopManagerInstance;

    int hr = 0;
    DWORD* textFlags = (DWORD*)(pThis + 80);
    BYTE* textFlags2 = (pThis + 280);
    DWORD CVisualFlags = *(pThis + 84);
    if (*textFlags & 0x1000) {
        LOGFONT font = *(LOGFONT*)(pThis + 296);
        RECT fillbox = { 0 };
        D2D1_SIZE_F size = { 0 };
        IWICImagingFactory* imagingfactory = uDwm::CDesktopManager::s_pDesktopManagerInstance->GetWICFactory();
        IWICBitmap* bitmap{};
        ID2D1RenderTarget* target{};
        IMiosD2D1RenderTarget* miostarget{};
        IDWriteTextFormat* textformat{};
        IDWriteTextLayout* textlayout{};
        ID2D1SolidColorBrush* textbrush{};
        ID2D1SolidColorBrush* shadowbrush{};
        IDWriteInlineObject* trimmingsign{};
        ID2D1Bitmap* atlasbitmap{};
        DWRITE_TEXT_METRICS metrics;
        uDwm::CCompositor* compositor = uDwm::CDesktopManager::s_pDesktopManagerInstance->GetCompositor();
        uDwm::CBaseTransformProxy* transformProxy = nullptr;
        uDwm::CBitmapSource* bmpsrc = nullptr;
        uDwm::CDrawImageInstruction* imageinstruction = nullptr;
        uDwm::CPushTransformInstruction* transforminstruction = nullptr;
        uDwm::CPopInstruction* popinstruction = nullptr;
        HTHEME hTheme = *(HTHEME*)(CDM_pDesktopManagerInstance + 608);
        RECT atlasrect = { 0 };
        MARGINS sizingmargins = { 0 };
        MARGINS contentmargins = { 0 };
        D2D1_RECT_F glowdrawrect = { 0 };
        D2D1_RECT_F glowsrcrect = { 0 };
        D2D1_SIZE_F glowscale = { 1, 1 };
        bool glowsizechanged = false;

        LPCWSTR string = *(LPCWSTR*)(pThis + 288);
        TEXTEX* textex = This->GetTextEx();
        if (*(BYTE*)(pThis + 411) == false) {
            This->SetTextEx();
            TEXTEX* textex = This->GetTextEx();
            *(pThis + 411) = true;
        }
        fillbox.right = *(int*)(pThis + 120);
        fillbox.bottom = *(int*)(pThis + 124);

        if (fillbox.left != textex->fillboxSize.left ||
            fillbox.top != textex->fillboxSize.top ||
            fillbox.right != textex->fillboxSize.right ||
            fillbox.bottom != textex->fillboxSize.bottom)
            textex->render = true;
        if (textex->render) {
            This->ReleaseResources(This);
            if (fillbox.right > 0 && fillbox.bottom > 0) {
                textex->fillboxSize = fillbox;

                if (CVisualFlags & 1) {
                    //OffsetRect(&fillbox, -(fillbox.right), 0);
                    //OffsetRect(&fillbox, -(textex->textInset.cxLeftWidth), 0);
                    //textex->textInset.cxLeftWidth = 0;
                }

                if (string) {
                    //int stringlength = wcslen(string);
                    //DWRITE_TEXT_RANGE range = { 0, stringlength };

                    hr = imagingfactory->CreateBitmap(
                        fillbox.right - fillbox.left,
                        fillbox.bottom - fillbox.top,
                        GUID_WICPixelFormat32bppPBGRA,
                        WICBitmapCacheOnDemand,
                        &bitmap
                    );
                    if (hr < 0)
                        goto release;

                    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT);
                    hr = d2dFactory->CreateWicBitmapRenderTarget(bitmap, props, &target);
                    if (hr < 0)
                        goto release;

                    size = target->GetSize();
                    size.width -= (textex->textInset.cxLeftWidth + textex->textInset.cxRightWidth);
                    if (size.width > 0) {

                        /*D2D1_COLOR_F colorText = {
                            awmsettings.colorTextActiveR,
                            awmsettings.colorTextActiveG,
                            awmsettings.colorTextActiveB,
                            1.0f
                        };*/
                        D2D1_COLOR_F colorText = textex->color;
                        hr = target->CreateSolidColorBrush(colorText, &textbrush);
                        if (hr < 0)
                            goto release;

                        /*D2D1_COLOR_F colorShadow = {
                            awmsettings.colorTextShadowActiveR,
                            awmsettings.colorTextShadowActiveG,
                            awmsettings.colorTextShadowActiveB,
                            awmsettings.colorTextShadowActiveA
                        };*/
                        D2D1_COLOR_F colorShadow = textex->shadowcolor;
                        hr = target->CreateSolidColorBrush(colorShadow, &shadowbrush);
                        if (hr < 0)
                            goto release;

                        /*DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
                        if (font.lfItalic)
                            style = DWRITE_FONT_STYLE_ITALIC;
                        float height = (float)-font.lfHeight;
                        if (height < 0) {
                            height = -height;
                        }
                        else if (height == 0) {
                            height = -(float)((*(LOGFONT*)(*CDM_pDesktopManagerInstance + CDM_CaptionFont)).lfHeight);
                        }
                        hr = dwritefactory->CreateTextFormat(
                            font.lfFaceName,
                            NULL,
                            (DWRITE_FONT_WEIGHT)font.lfWeight,
                            style,
                            DWRITE_FONT_STRETCH_NORMAL,
                            height,
                            L"en-us",
                            &textformat
                        );
                        if (hr < 0)
                            goto release;

                        textformat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                        hr = dwritefactory->CreateEllipsisTrimmingSign(textformat, &trimmingsign);
                        if (hr < 0)
                            goto release;
                        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, NULL, 0 };
                        hr = textformat->SetTrimming(&trimming, trimmingsign);
                        if (hr < 0)
                            goto release;
                        if (CVisualFlags & CVIS_FLAG_RTL) {
                            hr = textformat->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
                            if (hr < 0)
                                goto release;
                        }

                        if (awmsettings.textAlignment == AWM_TEXT_CENTER_ICONBUTTONS) {
                            textformat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                        }
                        else if (awmsettings.textAlignment == AWM_TEXT_RIGHT) {
                            textformat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                        }*/
                        if (!textex->textFormat) {
                            CText_CreateTextFormat(This, &font);
                        }
                        textformat = textex->textFormat;
                        textlayout = textex->textLayout;
                        if (!textlayout)
                            goto release;
                        /*hr = dwritefactory->CreateTextLayout(string, stringlength, textformat, size.width, size.height, &textlayout);
                        if (hr < 0)
                            goto release;
                        hr = textlayout->SetStrikethrough(font.lfStrikeOut, range);
                        if (hr < 0)
                            goto release;
                        hr = textlayout->SetUnderline(font.lfUnderline, range);
                        if (hr < 0)
                            goto release;*/
                        hr = textlayout->SetMaxWidth(size.width);
                        if (hr < 0)
                            goto release;
                        hr = textlayout->SetMaxHeight(size.height);
                        if (hr < 0)
                            goto release;
                        if (CVisualFlags & 1) {
                            hr = textlayout->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
                            if (hr < 0)
                                goto release;
                        }

                        if (g_textAlign == TextAlign::CenterUx) {
                            textlayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                        }
                        else if (g_textAlign == TextAlign::Right) {
                            textlayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                        }
                        hr = textlayout->GetMetrics(&metrics);
                        if (hr < 0)
                            goto release;

                        MARGINS insetFromParent = *(MARGINS*)(pThis + 128);
                        DWORD fullTBWidth = fillbox.right;

                        // This fixes the bug described below:
                        if (insetFromParent.cxLeftWidth == 0x7fffffff || insetFromParent.cxRightWidth == 0x7fffffff)
                            fullTBWidth = textex->tbWidth;
                        textex->tbWidth = fullTBWidth;

                        float xoffset = 0.0f;
                        float yoffset = (size.height - metrics.height - 1.5f) / 2.0f;
                        if (g_textAlign == TextAlign::CenterW8) {
                            xoffset = (float)floor((fullTBWidth - metrics.width) / 2.0f + 0.5f);

                            // BUG! For some reason the inset is set to 0x7fffffff while minimizing. Make fullTBWidth 
                            // be a property in the extra CText object that will be created, to avoid this issue, as 
                            // it will be taken directly from the window itself (in UpdateNCAreaPositionsAndSizes)

                            //xoffset -= (float)insetFromParent.cxLeftWidth;
                            xoffset -= 1;
                            xoffset -= textex->textInset.cxLeftWidth;
                            if (xoffset < 0)
                                xoffset = 0;
                            if (xoffset + metrics.width >= size.width) {
                                xoffset = (size.width - metrics.width) / 2.0f;

                                textlayout->SetMaxWidth(fullTBWidth);
                                DWRITE_TEXT_METRICS fullmetrics;
                                textlayout->GetMetrics(&fullmetrics);
                                if (size.width < fullmetrics.width) {
                                    xoffset = 0.0f;
                                }
                                textlayout->SetMaxWidth(size.width);
                            }
                        }
                        else if (g_textAlign == TextAlign::CenterDulappy) {
                            xoffset = ((float)fullTBWidth - metrics.width) / 2.0f;
                            //xoffset -= (float)insetFromParent.cxLeftWidth;
                            xoffset -= 1;
                            xoffset -= textex->textInset.cxLeftWidth;
                            if (xoffset + metrics.width >= size.width)
                                xoffset = size.width - metrics.width;
                            if (xoffset < 0)
                                xoffset = 0;
                        }

                        // Ensure that the update flag will be set every time the window is resized.
                        *textFlags2 &= ~1;
                        // These two are now being used to store the CTextEx object's address.
                        //*(DWORD*)(pThis + CTxt_LimRight) = 0;
                        //*(DWORD*)(pThis + CTxt_LimBottom) = 0;

                        hr = GetMiosThemeBitmapProps(
                            hTheme,
                            45,
                            0,
                            &atlasrect,
                            &sizingmargins,
                            &contentmargins,
                            0,
                            true
                        );
                        hr = target->CreateBitmapFromWicBitmap(DWMAtlas, &atlasbitmap);
                        if (hr < 0)
                            goto release;

                        if (CVisualFlags & 1) {
                            xoffset = -xoffset;
                            xoffset -= textex->textInset.cxLeftWidth;
                        }
                        else if (metrics.left < 0) {
                            xoffset -= metrics.left;
                        }
                        glowsrcrect.left = atlasrect.left;
                        glowsrcrect.top = atlasrect.top;
                        glowsrcrect.right = atlasrect.right;
                        glowsrcrect.bottom = atlasrect.bottom;

                        glowdrawrect.left = xoffset - contentmargins.cxLeftWidth;
                        glowdrawrect.right = xoffset + metrics.width + contentmargins.cxRightWidth;
                        glowdrawrect.top = (int)(yoffset + 0.75f - (float)contentmargins.cyTopHeight);
                        glowdrawrect.bottom = yoffset + metrics.height + contentmargins.cyBottomHeight;
                        if (metrics.left > 0) {
                            glowdrawrect.left += metrics.left;
                            glowdrawrect.right += metrics.left;
                        }
                        if (glowdrawrect.bottom - glowdrawrect.top < sizingmargins.cyBottomHeight + sizingmargins.cyTopHeight) {
                            float heightnew = sizingmargins.cyBottomHeight + sizingmargins.cyTopHeight;
                            glowscale.height = (float)(glowdrawrect.bottom - glowdrawrect.top) / heightnew;
                            glowdrawrect.bottom = glowdrawrect.top + heightnew;
                            glowsizechanged = true;
                        }
                        if (glowdrawrect.right - glowdrawrect.left < sizingmargins.cxRightWidth + sizingmargins.cxLeftWidth) {
                            float widthnew = sizingmargins.cxRightWidth + sizingmargins.cxLeftWidth;
                            glowscale.width = (float)(glowdrawrect.right - glowdrawrect.left) / widthnew;
                            glowdrawrect.right = glowdrawrect.left + widthnew;
                            glowsizechanged = true;
                        }

                        target->BeginDraw();
                        //fwprintf(stream, L"%i\n", insetFromParent.cxLeftWidth);
                        D2D1_POINT_2F start = { xoffset , yoffset };
                        D2D1_POINT_2F startshadow = start;
                        startshadow.x += g_textShadowOffsetX;
                        startshadow.y += g_textShadowOffsetY;

                        D2D1_MATRIX_3X2_F trnsfrmmatrix = D2D1::Matrix3x2F::Translation(textex->textInset.cxLeftWidth, 0);
                        target->SetTransform(trnsfrmmatrix);

                        if (glowsizechanged) {
                            D2D1_POINT_2F point = { glowdrawrect.left, glowdrawrect.top };
                            D2D1_MATRIX_3X2_F sizematrix = D2D1::Matrix3x2F::Scale(glowscale, point);
                            target->SetTransform(trnsfrmmatrix * sizematrix);
                        }

                        miostarget = (IMiosD2D1RenderTarget*)target;
                        miostarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                        miostarget->DrawNineSliceBitmap(
                            atlasbitmap,
                            &glowdrawrect,
                            textex->glowopacity,
                            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                            &glowsrcrect,
                            &sizingmargins
                        );
                        miostarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

                        target->SetTransform(D2D1::Matrix3x2F::Scale(1, 1) * trnsfrmmatrix);

                        miostarget->PushAxisAlignedClip({ 0, 0, size.width, size.height }, D2D1_ANTIALIAS_MODE_ALIASED);
                        target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
                        target->DrawTextLayout(startshadow, textlayout, shadowbrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
                        target->DrawTextLayout(start, textlayout, textbrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                        target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
                        miostarget->PopAxisAlignedClip();

                        hr = target->EndDraw();
                        if (hr < 0)
                            goto release;

                        /*transformProxy = *(CBaseTransformProxy**)(pThis + CTxt_TransformProxy);
                        hr = CCompositor_CreateMatrixTransformProxy(compositor, &transformProxy);
                        if (hr < 0)
                            goto release;
                        hr = CPushTransformInstruction_Create(transformProxy, &transforminstruction);
                        if (hr < 0)
                            goto release;
                        hr = CRenderDataVisual_AddInstruction(pThis, transforminstruction);
                        if (hr < 0)
                            goto release;*/

                        hr = uDwm::CBitmapSource::Create(bitmap, 0, &bmpsrc);

                        if (hr < 0)
                            goto release;
                        hr = uDwm::CDrawImageInstruction::Create(bmpsrc, &fillbox, &imageinstruction);
                        if (hr < 0)
                            goto release;
                        hr = This->AddInstruction(imageinstruction);
                        if (hr < 0)
                            goto release;

                        /*hr = CPopInstruction_Create(&popinstruction);
                        if (hr < 0)
                            goto release;
                        hr = CRenderDataVisual_AddInstruction(pThis, popinstruction);*/
                    }
                    textex->render = false;
                }
            }
        }
    release:
        if (bitmap)
            bitmap->Release();
        if (target)
            target->Release();
        if (textbrush)
            textbrush->Release();
        if (shadowbrush)
            shadowbrush->Release();
        //if (textformat)
        //    textformat->Release();
        //if (textlayout)
        //    textlayout->Release();
        if (trimmingsign)
            trimmingsign->Release();
        if (atlasbitmap)
            atlasbitmap->Release();
        if (bmpsrc)
            bmpsrc->Release();
        //if (transforminstruction)
        //    CBaseObject_Release(transforminstruction);
        if (imageinstruction)
            imageinstruction->Release();
        //if (popinstruction)
        //    CBaseObject_Release(popinstruction);
    }
    *textFlags &= ~0x1000;
    return hr;
}
uDwm::CText* STDMETHODCALLTYPE CaptionTextHandler::MyCText_CText(uDwm::CText* This)
{
	uDwm::CText* textobj = g_CText_CText_Org(This);
    This->SetTextEx();
    TEXTEX* textex = This->GetTextEx();
	// This can be checked so that the object is created within various text-related functions.
	*((BYTE*)textobj + 411) = true;
	textex->render = true;
	return textobj;
}
uDwm::CText* STDMETHODCALLTYPE CaptionTextHandler::MyCText_Destroy(uDwm::CText* This, UINT someFlags)
{
	if (*((BYTE*)This + 411)) {
		TEXTEX* textex = This->GetTextEx();
		if (textex->textFormat)
			textex->textFormat->Release();
		if (textex->textLayout)
			textex->textLayout->Release();
		//free(This.textEx);
	}
	uDwm::CText* textobj = g_CText_Destroy_Org(This, someFlags);
	return textobj;
}
long STDMETHODCALLTYPE CaptionTextHandler::MyCText_SetText(uDwm::CText* This, wchar_t* str)
{
	long hr = g_CText_SetText_Org(This, str);
    TEXTEX* textex = This->GetTextEx();
	if (*((BYTE*)This + 411)) {
		textex->render = true;
		CText_CreateTextLayout(This);
	}
	return hr;
}
void STDMETHODCALLTYPE CaptionTextHandler::MyCText_SetFont(uDwm::CText* This, LOGFONTW* font)
{
    TEXTEX* textex = This->GetTextEx();
	if (memcmp(((BYTE*)This + 296), font, sizeof(LOGFONTW))) {
		if (*((BYTE*)This + 411)) {
			textex->render = true;
			CText_CreateTextFormat(This, font);
		}
	}
	g_CText_SetFont_Org(This, font);
}

void STDMETHODCALLTYPE CaptionTextHandler::MyCText_SetColor(uDwm::CText* This, COLORREF color)
{
    return;
}
void STDMETHODCALLTYPE CaptionTextHandler::MyCText_SetBackgroundColor(uDwm::CText* This, COLORREF color)
{
    return;
}

HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCTopLevelWindow_UpdateWindowVisuals(uDwm::CTopLevelWindow* This)
{
    int rv = g_CTopLevelWindow_UpdateWindowVisuals_Org(This);
    D2D1_COLOR_F color;
    D2D1_COLOR_F colorShadow;
    float glowOpacity = 0.0f;
    BYTE* windowdata = *(BYTE**)((BYTE*)This + 728);
    RECT winrc = *(RECT*)(windowdata + 180);
    uDwm::CText* textobj = *(uDwm::CText**)((BYTE*)This + 520);
    if (textobj) {
        if (*((BYTE*)textobj + 411) == false) {
            textobj->SetTextEx();
            TEXTEX* textex = textobj->GetTextEx();
            *((BYTE*)textobj + 411) = true;
            textex->render = true;
        }
        TEXTEX* textex = textobj->GetTextEx();
        if (This->TreatAsActiveWindow()) {
            color.r = 1.0f;
            color.g = 1.0f;
            color.b = 1.0f;
            color.a = 1.0f;
            colorShadow.r = 0.0f;
            colorShadow.g = 0.0f;
            colorShadow.b = 0.0f;
            colorShadow.a = 1.0f;
            glowOpacity = 1.0f;
            if (!textex->isActive) {
                textex->render = true;
                textex->isActive = true;
            }
        }
        else {
            color.r = 1.0f;
            color.g = 1.0f;
            color.b = 1.0f;
            color.a = 1.0f;
            colorShadow.r = 0.0f;
            colorShadow.g = 0.0f;
            colorShadow.b = 0.0f;
            colorShadow.a = 1.0f;
            glowOpacity = 1.0f;
            if (textex->isActive) {
                textex->render = true;
                textex->isActive = false;
            }
        }
        BYTE* textFlags2 = ((BYTE*)textobj + 280);
        *textFlags2 &= ~1;
        textex->color = color;
        textex->shadowcolor = colorShadow;
        textex->glowopacity = glowOpacity;
    }

    return rv;
}

void CaptionTextHandler::UpdateConfiguration(ConfigurationFramework::UpdateType type)
{
	if (type & ConfigurationFramework::UpdateType::Backdrop)
	{
		g_centerCaption = static_cast<bool>(ConfigurationFramework::DwmGetDwordFromHKCUAndHKLM(L"CenterCaption", FALSE));
		g_textGlowSize = standardGlowSize;
		auto glowSize = ConfigurationFramework::DwmTryDwordFromHKCUAndHKLM(L"TextGlowSize");
		if (!glowSize.has_value())
		{
			glowSize = ConfigurationFramework::DwmTryDwordFromHKCUAndHKLM(L"TextGlowMode");
			if (glowSize.has_value())
			{
				g_textGlowSize = HIWORD(glowSize.value());
			}
		}
		else
		{
			g_textGlowSize = glowSize.value();
		}
		g_captionColor = ConfigurationFramework::DwmTryDwordFromHKCUAndHKLM(L"ColorizationColorCaption");
	}
}

HRESULT CaptionTextHandler::Startup()
{
    /*DWORD value{0ul};
    LOG_IF_FAILED_WITH_EXPECTED(
        wil::reg::get_value_dword_nothrow(
            ConfigurationFramework::GetDwmKey(),
            L"DisabledHooks",
            reinterpret_cast<DWORD*>(&value)
        ),
        HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
    );
    g_disableTextHooks = (value & 1) != 0;*/
	
	g_disableTextHooks = 1;

	if (!g_disableTextHooks)
	{
		if (os::buildNumber < os::build_w11_22h2)
		{
            RETURN_IF_FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory));
			RETURN_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwriteFactory));

            uDwm::GetAddressFromSymbolMap("CTopLevelWindow::UpdateWindowVisuals", g_CTopLevelWindow_UpdateWindowVisuals_Org);

            uDwm::GetAddressFromSymbolMap("CDesktopManager::LoadTheme", g_CDesktopManager_LoadTheme_Org);
            uDwm::GetAddressFromSymbolMap("CDesktopManager::UnloadTheme", g_CDesktopManager_UnloadTheme_Org);

            uDwm::GetAddressFromSymbolMap("CText::InitializeVisualTreeClone", g_CText_InitializeVisualTreeClone_Org);
			uDwm::GetAddressFromSymbolMap("CText::ValidateResources", g_CText_ValidateResources_Org);
			uDwm::GetAddressFromSymbolMap("CText::CText", g_CText_CText_Org);
			uDwm::GetAddressFromSymbolMap("CText::`scalar deleting destructor'", g_CText_Destroy_Org);
			uDwm::GetAddressFromSymbolMap("CText::SetText", g_CText_SetText_Org);
			uDwm::GetAddressFromSymbolMap("CText::SetFont", g_CText_SetFont_Org);
            uDwm::GetAddressFromSymbolMap("CText::SetColor", g_CText_SetColor_Org);
            uDwm::GetAddressFromSymbolMap("CText::SetBackgroundColor", g_CText_SetBackgroundColor_Org);

			return HookHelper::Detours::Write([]()
			{
                HookHelper::Detours::Attach(&g_CDesktopManager_LoadTheme_Org, MyCDesktopManager_LoadTheme);
                HookHelper::Detours::Attach(&g_CDesktopManager_UnloadTheme_Org, MyCDesktopManager_UnloadTheme);
                HookHelper::Detours::Attach(&g_CText_CText_Org, MyCText_CText);
				HookHelper::Detours::Attach(&g_CText_Destroy_Org, MyCText_Destroy);
                HookHelper::Detours::Attach(&g_CText_SetText_Org, MyCText_SetText);
				HookHelper::Detours::Attach(&g_CText_SetFont_Org, MyCText_SetFont);
                HookHelper::Detours::Attach(&g_CText_SetColor_Org, MyCText_SetColor);
                HookHelper::Detours::Attach(&g_CText_SetBackgroundColor_Org, MyCText_SetBackgroundColor);
				HookHelper::Detours::Attach(&g_CText_ValidateResources_Org, MyCText_ValidateResources);
                HookHelper::Detours::Attach(&g_CText_InitializeVisualTreeClone_Org, MyCText_InitializeVisualTreeClone);
                HookHelper::Detours::Attach(&g_CTopLevelWindow_UpdateWindowVisuals_Org, MyCTopLevelWindow_UpdateWindowVisuals);
			});
		}
	}

    return S_OK;
}
void CaptionTextHandler::Shutdown()
{
	if (!g_disableTextHooks)
	{
		if (os::buildNumber < os::build_w11_22h2)
		{
			HookHelper::Detours::Write([]()
			{
                HookHelper::Detours::Detach(&g_CDesktopManager_LoadTheme_Org, MyCDesktopManager_LoadTheme);
                HookHelper::Detours::Detach(&g_CDesktopManager_UnloadTheme_Org, MyCDesktopManager_UnloadTheme);
				HookHelper::Detours::Detach(&g_CText_CText_Org, MyCText_CText);
				HookHelper::Detours::Detach(&g_CText_Destroy_Org, MyCText_Destroy);
                HookHelper::Detours::Detach(&g_CText_SetText_Org, MyCText_SetText);
				HookHelper::Detours::Detach(&g_CText_SetFont_Org, MyCText_SetFont);
                HookHelper::Detours::Detach(&g_CText_SetColor_Org, MyCText_SetColor);
                HookHelper::Detours::Detach(&g_CText_SetBackgroundColor_Org, MyCText_SetBackgroundColor);
                HookHelper::Detours::Detach(&g_CText_ValidateResources_Org, MyCText_ValidateResources);
                HookHelper::Detours::Detach(&g_CText_InitializeVisualTreeClone_Org, MyCText_InitializeVisualTreeClone);
                HookHelper::Detours::Detach(&g_CTopLevelWindow_UpdateWindowVisuals_Org, MyCTopLevelWindow_UpdateWindowVisuals);
			});

			g_textVisual = nullptr;
			g_textWidth = 0;
			g_textHeight = 0;
		}
	}
}
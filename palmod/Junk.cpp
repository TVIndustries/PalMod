#include "StdAfx.h"
#include ".\junk.h"

//This is used for the edit commands
#include "resource.h"

CPen CJunk::PIndexHL;
CPen CJunk::PIndexSL;
CPen CJunk::PIndexMHL;
CPen CJunk::PIndexBG;

int CJunk::nWidthMax = PAL_MAXWIDTH;

BOOL bTest = CJunk::InitPen();

UCHAR CJunk::Toggle(UCHAR& tVar)
{
    tVar = !tVar;

    return tVar;
}

void CJunk::ClearSelView()
{
    for (int i = 0; i < iWorkingAmt; i++)
    {
        SelView[i] = FALSE;
    }

    iHLAmt = 0;
}

void CJunk::ClearSelected()
{
    for (int i = 0; i < iWorkingAmt; i++)
    {
        Selected[i] = FALSE;
    }

    SingleSelect = -1;

    iSelAmt = 0;
}

void CJunk::ClearHighlighted()
{
    for (int i = 0; i < iWorkingAmt; i++)
    {
         Highlighted[i] = 0;
    }

    //iHLAmt = 0;
}

void CJunk::LoadDefaultPal()
{
    for (int i = 0; i < iPalH * iPalW; i++)
    {
        BasePal[i] = RGB((F_R + 1) * i / 13, (F_G + 1) * i / 13, (F_B + 1) * i / 13);
    }
}

BOOL CJunk::InitNewSize(int nNewAmt, COLORREF* rgNewPal)
{
    //Clear the current variables/pointers/arrays etc
    if (nNewAmt != nCurrAmt)
    {
        if (nNewAmt > PAL_MAXAMT)
        {
            OutputDebugString("ERROR: Our color table can only show 64 colors!\n");
            return FALSE;
        }

        if (!bFirstInit)
        {
            CleanUp();
        }
        else
        {
            bFirstInit = FALSE;
        }

        //Set new width/height
        iPalW = nNewAmt > nWidthMax ? nWidthMax : nNewAmt;

        if (nNewAmt > nWidthMax)
        {
            iPalH = nNewAmt / 8 + (nNewAmt % nWidthMax ? 1 : 0);
        }
        else
        {
            iPalH = 1;
        }

        //Set control width/height
        iBaseW = (iPalW * PAL_SQ_SZ) + ((iPalW * BDR_SZ) + BDR_SZ);
        iBaseH = (iPalH * PAL_SQ_SZ) + ((iPalH * BDR_SZ) + BDR_SZ);

        //Set the working amount
        iWorkingAmt = nNewAmt;

        bUnused = (iPalH > 1 ? iWorkingAmt % iPalW : 0);

        if (bUnused)
        {
            int nSrcIndex = (iWorkingAmt % nWidthMax) + ((iPalH - 1) * nWidthMax);
            int nDstIndex = ((iPalH * nWidthMax) - 1);

            rUnused.top = (BDR_SZ * ((nSrcIndex / iPalW) + 1)) + ((PAL_SQ_SZ) * (nSrcIndex / iPalW));
            rUnused.left = (BDR_SZ * ((nSrcIndex % iPalW) + 1)) + ((PAL_SQ_SZ) * (nSrcIndex % iPalW));
            rUnused.right = (BDR_SZ * ((nDstIndex % iPalW) + 2)) + (PAL_SQ_SZ * ((nDstIndex % iPalW) + 1));
            rUnused.bottom = (BDR_SZ * ((nDstIndex / iPalW) + 2)) + (PAL_SQ_SZ * ((nDstIndex / iPalW) + 1));
        }

        //Create Selected, Highlighted and SelView
        Highlighted = new UCHAR[iPalW * iPalH];
        Selected = new UCHAR[iPalW * iPalH];
        SelView = new UCHAR[iPalW * iPalH];

        ClearSelView();
        ClearSelected();
        ClearHighlighted();

        ProcBaseBMP();

        //Set the palette
        BasePal = rgNewPal;

        UpdateIndexAll();

        nCurrAmt = nNewAmt;
    }
    else
    {
        BasePal = rgNewPal;

        UpdateIndexAll();
    }

    return TRUE;
}

void CJunk::CleanUp()
{
    //Remove Selected, SelView and Highlighted
    safe_delete_array(Selected);
    safe_delete_array(SelView);
    safe_delete_array(Highlighted);

    //Remove palette

    /*
    safe_delete_array(BasePal);
    */
}

void CJunk::NotifyParent(int iCustomMessage)
{
    static NMHDR myhdr;

    myhdr.hwndFrom = GetSafeHwnd();
    myhdr.idFrom = nArrayIndex;
    myhdr.code = iCustomMessage;

    GetParent()->PostMessage(WM_NOTIFY, 0, (LPARAM)&myhdr);
}

void CJunk::SetIndexPen(int nIndex, int pFlag)
{
    switch (pFlag)
    {
    case FLAG_HL:
        dcBaseDC.SelectObject(&PIndexHL);
        break;
    case FLAG_SL:
        dcBaseDC.SelectObject(&PIndexSL);
        break;
    case FLAG_MH:
        dcBaseDC.SelectObject(&PIndexMHL);
        break;
    case FLAG_DE:
        dcBaseDC.SelectObject(&PIndexBG);
        break;
    }
}

BOOL CJunk::InitPen()
{
    PIndexHL.CreatePen(0, 1, RGB(HL_R, HL_G, HL_B));
    PIndexSL.CreatePen(0, 1, RGB(SL_R, SL_G, SL_B));
    PIndexMHL.CreatePen(0, 1, RGB(MH_R, MH_G, MH_B));
    PIndexBG.CreatePen(0, 1, RGB(BG_R, BG_G, BG_B));

    return TRUE;
}

CJunk::CJunk()
{
    RegisterWindowClass();
}

CJunk::~CJunk(void)
{
    //delete (dcPaintDC);
    CleanUp();
}

BEGIN_MESSAGE_MAP(CJunk, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_TIMER()
    ON_WM_KEYDOWN()
    ON_WM_KEYUP()
    ON_WM_COPYDATA()
    ON_WM_CREATE()
    ON_WM_RBUTTONDOWN()
    ON_WM_INITMENUPOPUP()
    ON_WM_MENUSELECT()
END_MESSAGE_MAP()

BOOL CJunk::RegisterWindowClass()
{
    WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if (!(::GetClassInfo(hInst, JUNK_CLASSNAME, &wndcls)))
    {
        // otherwise we need to register a new class
        wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc = ::DefWindowProc;
        wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
        wndcls.hInstance = hInst;
        wndcls.hIcon = NULL;
        wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndcls.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wndcls.lpszMenuName = NULL;
        wndcls.lpszClassName = JUNK_CLASSNAME;

        if (!AfxRegisterClass(&wndcls))
        {
            AfxThrowResourceException();
            return FALSE;
        }
    }

    return TRUE;
}

void CJunk::OnPaint()
{
    InitDC();
    UpdateCtrl(FALSE);

    CWnd::OnPaint();
}

BOOL CJunk::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

BOOL CJunk::ProcBaseBMP()
{
    if (hBmp && !DeleteObject(hBmp))
    {
        OutputDebugString("ERROR: Could not delete old hBmp\n");
    }

    Bmpi.bmiHeader.biWidth = iBaseW;
    Bmpi.bmiHeader.biHeight = -iBaseH;
    Bmpi.bmiHeader.biPlanes = 1;
    Bmpi.bmiHeader.biBitCount = 32;
    Bmpi.bmiHeader.biCompression = BI_RGB;
    Bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    //HBITMAP hTempBmp;
    //dcBaseDC.SelectObject(&hTempBmp);

    hBmp = CreateDIBSection(dcBaseDC.GetSafeHdc(), &Bmpi, DIB_RGB_COLORS, (void**)&pBmpData, NULL, 0);

    if (!bFirstDCInit)
    {
        dcBaseDC.SelectObject(hBmp);
        dcBaseDC.SelectStockObject(NULL_BRUSH);
    }

    DrawBG();

    return TRUE;
}

void CJunk::ClearBaseBMP()
{
    safe_delete_array(pBmpData);
}

void CJunk::InitDC()
{
    if (bFirstDCInit)
    {
        dcPaintDC = new CPaintDC(this);

        //Init base
        dcBaseDC.CreateCompatibleDC(dcPaintDC);
        dcBaseDC.SelectObject(hBmp);
        dcBaseDC.SelectStockObject(NULL_BRUSH);

        bFirstDCInit = FALSE;

        UpdateIndexAll();
    }
}

void CJunk::DrawBG()
{
    if (!bFirstDCInit)
    {
        CRect rBGRect(0, 0, iBaseW, iBaseH);

        dcBaseDC.FillSolidRect(&rBGRect, RGB(BG_R, BG_G, BG_B));

        //Draw the unused palette indexes
        if (bUnused)
        {
            dcBaseDC.FillSolidRect(&rUnused, GetSysColor(COLOR_3DFACE));
        }
    }
}

//BOOL CJunk::DrawBaseBorder()
//{
//    dcPaintDC->BitBlt(0,0,iBaseW,iBaseH, &dcBaseDC, 0, 0, SRCCOPY);
//
//    return TRUE;
//}

void CJunk::SelectAll()
{
    if (Selected)
    {
        for (int i = 0; i < iWorkingAmt; i++)
        {
            Selected[i] = TRUE;
        }
    }
}

void CJunk::UpdateIndex(int index)
{
    if (bEnabled && !bFirstDCInit)
    {
        if (index < iWorkingAmt)
        {
            rIndexRect.top = (BDR_SZ * ((index / iPalW) + 1)) + ((PAL_SQ_SZ) * (index / iPalW));
            rIndexRect.left = (BDR_SZ * ((index % iPalW) + 1)) + ((PAL_SQ_SZ) * (index % iPalW));
            rIndexRect.right = (BDR_SZ * ((index % iPalW) + 1)) + (PAL_SQ_SZ * ((index % iPalW) + 1));
            rIndexRect.bottom = (BDR_SZ * ((index / iPalW) + 1)) + (PAL_SQ_SZ * ((index / iPalW) + 1));

            CustomFillRect(&rIndexRect, (UINT8*)&BasePal[index]);
        }
    }
}

void CJunk::UpdateFace()
{
    //return;
    static int nNewCt = 0;
    static int nDelCt = 0;

    if (bEnabled)
    {
        BOOL bDraw;
        RECT* rSqRct = new RECT[iWorkingAmt];

        nNewCt++;

        for (int index = 0; index < iWorkingAmt; index++)
        {
            SetIndexPen(index, FLAG_DE);

            rSqRct[index].top = (BDR_SZ * ((index / iPalW) + 1)) + ((PAL_SQ_SZ) * (index / iPalW));
            rSqRct[index].left = (BDR_SZ * ((index % iPalW) + 1)) + ((PAL_SQ_SZ) * (index % iPalW));
            rSqRct[index].right = (BDR_SZ * ((index % iPalW) + 1)) + (PAL_SQ_SZ * ((index % iPalW) + 1));
            rSqRct[index].bottom = (BDR_SZ * ((index / iPalW) + 1)) + (PAL_SQ_SZ * ((index / iPalW) + 1));

            InflateRect(&rSqRct[index], 1, 1);

            dcBaseDC.Rectangle(&rSqRct[index]);
        }

        for (int index = 0; index < iWorkingAmt; index++)
        {
            bDraw = TRUE;

            if (Selected[index])
            {
                SetIndexPen(index, FLAG_SL);
            }
            else
            {
                bDraw = FALSE;
            }

            if (bDraw)
            {
                dcBaseDC.Rectangle(&rSqRct[index]);
            }
        }

        for (int index = 0; index < iWorkingAmt; index++)
        {
            bDraw = TRUE;

            if (SelView[index])
            {
                SetIndexPen(index, FLAG_MH);
            }
            else
            {
                bDraw = FALSE;
            }

            if (bDraw)
            {
                dcBaseDC.Rectangle(&rSqRct[index]);
            }
        }

        for (int index = 0; index < iWorkingAmt; index++)
        {
            bDraw = TRUE;

            if (Highlighted[index])
            {
                SetIndexPen(index, FLAG_HL);
            }
            else
            {
                bDraw = FALSE;
            }

            if (bDraw)
            {
                dcBaseDC.Rectangle(&rSqRct[index]);
            }
        }

        delete[] rSqRct;
        nDelCt++;
    }
}

void CJunk::CustomFillRect(RECT* lpRect, UINT8* crSrcCol)
{
    int nSqW = lpRect->right - lpRect->left;
    int nSqH = lpRect->top - lpRect->bottom;

    UINT8* pDstImgData = (UINT8*)pBmpData;

    double fpDstA2 = (1.0f - ((double)crSrcCol[3]) / 255.0f);
    double fpDstA1 = 1.0f - fpDstA2;

    for (int y = lpRect->top * 4; y < lpRect->bottom * 4; y += 4)
    {
        for (int x = lpRect->left * 4; x < lpRect->right * 4; x += 4)
        {
            UINT8* crDstCol = const_cast<UINT8*>((UINT8*)&JUNK_BG[(y % JUNK_BG_H) * JUNK_BG_W + (x % JUNK_BG_W)]);

            pDstImgData[(y * iBaseW) + x + 2] = aaadd((fpDstA1 * (double)crSrcCol[0]), (fpDstA2 * (double)crDstCol[0]));
            pDstImgData[(y * iBaseW) + x + 1] = aaadd((fpDstA1 * (double)crSrcCol[1]), (fpDstA2 * (double)crDstCol[1]));
            pDstImgData[(y * iBaseW) + x] = aaadd((fpDstA1 * (double)crSrcCol[2]), (fpDstA2 * (double)crDstCol[2]));
        }
    }
}

void CJunk::UpdateIndexAll()
{
    for (int nCtr = 0; nCtr < iWorkingAmt; nCtr++)
    {
        UpdateIndex(nCtr);
    }
}

BOOL CJunk::UpdateCtrl(BOOL bUpdFace)
{
    if (bUpdFace)
    {
        UpdateFace();
    }

    dcPaintDC->BitBlt(0, 0, iBaseW, iBaseH, &dcBaseDC, 0, 0, SRCCOPY);

    return TRUE;
}

void CJunk::OnMouseMove(UINT nFlags, CPoint point)
{
    if (!bEnabled)
    {
        return;
    }

    if ((xHLOld != -1 ) && (yHLOld != -1))
    {
        Highlighted[(yHLOld * iPalW) + xHLOld] = FALSE;
    }

    if (!bOverControl)
    {
        bOverControl = TRUE;

        SetTimer(10, 100, NULL);
    }

    CPoint PalIndex;

    if (!LButtonDown)
    {
        //bMulHL = FALSE;

        if (ProcessHovered(point, PalIndex))
        {
            if (!(PalIndex.y >= iPalH || PalIndex.x >= iPalW))
            {
                Highlighted[(PalIndex.y * iPalW) + PalIndex.x] = TRUE;
            }

            if (PalIndex.y != yHLOld || PalIndex.x != xHLOld)
            {
                iHighlightIndex = (PalIndex.x) + (PalIndex.y * iPalW);
                iHighlightx = PalIndex.x;
                iHighlighty = PalIndex.y;

                iHLAmt = 1;

                NotifyParent(CUSTOM_HLCHANGE);

                UpdateCtrl();
            }

            yHLOld = PalIndex.y;
            xHLOld = PalIndex.x;
        }
        else
        {
            if ((xHLOld != -1) && (yHLOld != -1))
            {
                Highlighted[(yHLOld * iPalW) + xHLOld] = FALSE;
            }
        }
    }
    else
    {
        if (ProcessHovered(point, PalIndex))
        {
            if ((yHLOld != PalIndex.y) || (xHLOld != PalIndex.x))
            {
                //bMulHL = TRUE;
                ClearSelView();

                if ((PalIndex.y == yInSelStart) && (PalIndex.x == xInSelStart))
                {
                    Highlighted[(PalIndex.y * iPalW) + PalIndex.x] = TRUE;
                    iHLAmt = 1;
                }
                else
                {
                    //Multiple selection of palette entries are based on lines instead of boxes
                    int ix, iy, jx, jy, x, y;
                    int ks, ke, kt;

                    ix = iy = jx = jy = x = y = ks = ke = kt = 0;

                    ks = (yInSelStart * iPalW) + xInSelStart;
                    ke = (PalIndex.y * iPalW) + PalIndex.x;

                    if (ks > ke)
                    {
                        kt = ks;
                        ks = ke;
                        ke = kt;
                    }

                    for (y = ks; y <= ke; y++)
                    {
                        SelView[y] = TRUE;
                        iHLAmt++;
                    }
                }

                NotifyParent(CUSTOM_HLCHANGE);

                xHLOld = PalIndex.x;
                yHLOld = PalIndex.y;

                UpdateCtrl();
            }
        }
    }

    CWnd::OnMouseMove(nFlags, point);
}

void CJunk::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (!bEnabled)
    {
        return;
    }

    SetCapture();

    LButtonDown = TRUE;

    CPoint PalIndex;

    if (ProcessHovered(point, PalIndex))
    {
        yInSelStart = PalIndex.y;
        xInSelStart = PalIndex.x;

        int nNewSingleSel = (yInSelStart * iPalW) + (xInSelStart);

        if (!(nFlags & MK_CONTROL))
        {
            if (nNewSingleSel != SingleSelect)
            {
                ClearSelected();
            }
            else
            {
                if (iSelAmt > 1)
                {
                    ClearSelected();
                }
            }
        }

        SingleSelect = nNewSingleSel;

        UpdateCtrl();

        bInCtrl = TRUE;
    }
    else
    {
        bInCtrl = FALSE;
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void CJunk::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (!bEnabled)
    {
        return;
    }

    ReleaseCapture();

    LButtonDown = FALSE;

    if (bInCtrl)
    {
        if (iHLAmt > 1)
        {
            for (int iy = 0; iy < iPalH; iy++)
            {
                for (int ix = 0; ix < iPalW; ix++)
                {
                    if (SelView[(iy * iPalW) + ix])
                    {
                        if (!Selected[(iy * iPalW) + ix])
                        {
                            Selected[(iy * iPalW) + ix] = TRUE;
                            iSelAmt++;
                        }

                        SelView[(iy * iPalW) + ix] = FALSE;
                    }
                }
            }

            iHLAmt = 0;

            NotifyParent(CUSTOM_MS);
        }
        else
        {

            if (Toggle(Selected[(yInSelStart * iPalW) + xInSelStart]))
            {
                iSelAmt++;
            }
            else
            {
                iSelAmt--;
            }

            SelView[(yInSelStart * iPalW) + xInSelStart] = FALSE;

            NotifyParent(CUSTOM_SS);
        }
    }

    UpdateCtrl();

    CWnd::OnLButtonUp(nFlags, point);
}

BOOL CJunk::ProcessHovered(CPoint hPoint, CPoint& PalPos)
{
    int x = hPoint.x;
    int y = hPoint.y;

    int posmod = PAL_SQ_SZ + BDR_SZ;

    int xIn = x / posmod;
    int yIn = y / posmod;

    if (xIn >= iPalW)
        return FALSE;

    if (yIn >= iPalH)
        return FALSE;

    if ((yIn * iPalW) + xIn >= iWorkingAmt)
        return FALSE;

    PalPos.x = xIn;
    PalPos.y = yIn;

    return TRUE;

}
void CJunk::OnTimer(UINT nIDEvent)
{
    CPoint p(GetMessagePos());
    ScreenToClient(&p);

    // Get the bounds of the control (just the client area)
    CRect rect;
    rect.top = 0;
    rect.left = 0;

    rect.right = iBaseW;
    rect.bottom = iBaseH;

    // Check the mouse is inside the control
    if (!rect.PtInRect(p))
    {
        // if not then stop looking...
        bOverControl = FALSE;
        KillTimer(10);

        yHLOld = -1;
        xHLOld = -1;

        ClearHighlighted();
        UpdateCtrl();

        NotifyParent(CUSTOM_HLCHANGE);
    }

    CWnd::OnTimer(nIDEvent);
}

void CJunk::OnRButtonDown(UINT nFlags, CPoint point)
{
    CMenu PopupMenu;

    if (PopupMenu.CreatePopupMenu())
    {
        RECT rWnd; GetWindowRect(&rWnd);
        point.x += rWnd.left;
        point.y += rWnd.top;

        bool canCopyOrPaste = false;
        for (int i = 0; i < iWorkingAmt; i++)
        {
            if (Selected[i])
            {
                canCopyOrPaste = true;
                break;
            }
        }

        PopupMenu.AppendMenu(canCopyOrPaste ? MF_ENABLED : MF_DISABLED, CUSTOM_COPY, "&Copy");
        PopupMenu.AppendMenu(canCopyOrPaste ? MF_ENABLED : MF_DISABLED, CUSTOM_PASTE, "&Paste");
        PopupMenu.AppendMenu(MF_SEPARATOR, 0, "");
        PopupMenu.AppendMenu(MF_ENABLED, CUSTOM_SALL, "Select &All");
        PopupMenu.AppendMenu(MF_ENABLED, CUSTOM_SNONE, "Select &None");

        int result = PopupMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, NULL);

        switch (result)
        {
        case CUSTOM_COPY:
        case CUSTOM_PASTE:
            NotifyParent(result);
            break;
        case CUSTOM_SALL:
            SelectAll();
            UpdateCtrl();
            break;
        case CUSTOM_SNONE:
            ClearSelected();
            UpdateCtrl();
            break;
        }
    }
    else
    {
        OutputDebugString("ERROR: Couldn't create popup menu.\n");
        // MessageBox("Error Creating Menu", "PalMod", MB_ICONERROR);
    }

    CWnd::OnRButtonDown(nFlags, point);
}

void CJunk::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    // TODO: Add your message handler code here
}

void CJunk::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    CWnd::OnMenuSelect(nItemID, nFlags, hSysMenu);

    // TODO: Add your message handler code here
}

BOOL CJunk::OnCommand(WPARAM wParam, LPARAM lParam)
{
    // TODO: Add your specialized code here and/or call the base class

    switch (LOWORD(wParam))
    {
    case CUSTOM_COPY:
    case CUSTOM_PASTE:
    case CUSTOM_SALL:
    case CUSTOM_SNONE:
    {
        NotifyParent(LOWORD(wParam));
    }
    break;
    }
    return CWnd::OnCommand(wParam, lParam);
}

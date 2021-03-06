#pragma once

#include "afxwin.h"
#include "Junk.h"

#define PALTOOL_CLASSNAME    _T("CPalTool")  // Window class name

#define MAX_PALETTE        16

#define PAL_TXT_SPACE    5

#define ID_PALTOOLSPIN    0xFF00
#define PT_PAGECHANGE    WM_USER - 100

struct sPalEntry
{
    CJunk* PaletteCtrl = nullptr;
    CHAR* szPalStr = nullptr;
    UCHAR bAvail;
};

class CPalTool : public CWnd
{
private:
    sPalEntry pPalEntry[MAX_PALETTE];
    CSize PalSize[MAX_PALETTE];

    BOOL bFirstPaint = TRUE;
    BOOL bSpinInit = TRUE;

    CFont BaseFont;
    bool m_fFontisBold = true;
    int nFontHeight = -1;

    int nNotifyCtrlIndex = 0;

    CSpinButtonCtrl m_PgSpin;

    CRect rClient;

    int nCurrPage = 0;
    int nPageAmt = 0;
    int nCurrPalAmt = 0;
    int nPrevPalAmt = -1;

    int nPalViewH = 0;
    UINT16 rgPalRedir[MAX_PALETTE + 1];

    void Init();

    void ClearBG(CPaintDC* PaintDC);
    void DrawText();

    void OnPalSelChange(int nCtrlId, BOOL bCurrPage = TRUE);

    void SendPalMsg(int nCtrlId, int nType);
    void SetFontToBold(bool beBold);

public:
    CPalTool(void);
    ~CPalTool(void);

    void CleanUp();

    void BeginSetPal();
    void EndSetPal();
    void SetPal(int nIndex, int nAmt, COLORREF* rgNewCol, CHAR* szNewPalStr);
    int GetNotifyIndex() { return nNotifyCtrlIndex; };
    CJunk* GetNotifyPal() { return pPalEntry[nNotifyCtrlIndex].PaletteCtrl; };
    CJunk* GetPalCtrl(int nIndex) { return pPalEntry[nIndex].bAvail ? pPalEntry[nIndex].PaletteCtrl : NULL; };

    void ResetNotifyIndex() { nNotifyCtrlIndex = 0; };

    BOOL CurrPalAvail() { return pPalEntry[nNotifyCtrlIndex].bAvail; };
    void ShowAvailPal();
    void UpdateCtrl();

    //void ResetNotifyPal(int nIndex){OnPalSelChange(0);SendPalMsg(nNotifyCtrlIndex);};

    BOOL RegisterWindowClass();

    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDeltaposSpin(NMHDR* pNMHDR, LRESULT* pResult);
    void SetNotifyFunc(int (*ExtPalSelChange)(int), int (*ExtPalHLChange)(int), int (*ExtPalMHL)(int));

public:
    void UpdateEveryIndex();
    void UpdateEveryCtrl();

protected:
    virtual void PreSubclassWindow();

public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};

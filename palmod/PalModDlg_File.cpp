#include "stdafx.h"
#include "PalMod.h"
#include "PalModDlg.h"

#include "Game\GameDef.h"

constexpr auto c_AppRegistryRoot = _T("Software\\knarxed\\PalMod");
constexpr auto c_strLastUsedPath = _T("LastUsedPath");
constexpr auto c_strLastUsedGFlag = _T("LastUsedGFlag");

void CPalModDlg::LoadGameDir(int nGameFlag, CHAR* szLoadDir)
{
    CGameClass* GameGet = GetHost()->GetLoader()->LoadDir(nGameFlag, szLoadDir);

    if (GameGet)
    {
        ClearGameVar();
        GetHost()->SetGameClass(GameGet);

        //Set the last used location
        SetLastUsedDirectory(szLoadDir, GetHost()->GetCurrGame()->GetGameFlag());

        //The game has loaded OK
        PostGameLoad();
    }
    else
    {
        CString strError;
        strError.LoadString(IDS_ERROR_LOADING_GAME);
        MessageBox(strError, GetAppName(), MB_ICONERROR);
    }

    //Since we loaded a DIR, show status of load
    SetStatusText(GetHost()->GetLoader()->GetLoadSaveStr());
}

void CPalModDlg::PostGameLoad()
{
    CPalModApp* ProgHost = GetHost();

    //Get the game's palette display width
    nPalWMax = ProgHost->GetCurrGame()->GetPalDisplayW();

    CString strDebugInfo;
    strDebugInfo.Format("CPalModDlg::PostGameLoad : Successfully loaded files for '%s'\n", g_GameFriendlyName[ProgHost->GetCurrGame()->GetGameFlag()]);
    OutputDebugString(strDebugInfo);

    //Set pal, img, and img ctrl pointers
    MainPalGroup = ProgHost->GetBasePal();
    ImgDispCtrl = ProgHost->GetImgDispCtrl();

    if (ProgHost->IsImgLoaded())
    {
        ImgFile = ProgHost->GetImgFile();
    }
    else
    {
        ImgFile = NULL;
    }

    //Get color plane data
    GetPlaneData();

    //Force the image to redisplay
    bForceImg = TRUE;

    //Init the first palette selection
    OnPalSelChange(0);

    //Enable the program
    Enable(TRUE);

    //Update the combo selection
    UpdateCombo();

    GetDlgItem(IDC_BCHECKMIX)->ShowWindow((ProgHost->GetCurrGame()->GetGameFlag() == MVC2_D) ? SW_SHOW : SW_HIDE);

    CPreviewDlg* PreviewDlg = GetHost()->GetPreviewDlg();

    if (!PreviewDlg->IsWindowVisible())
    {
        PreviewDlg->ShowWindow(SW_SHOW);
        PreviewDlg->m_ImgDisp.UpdateCtrl();
    }
}

void CPalModDlg::OnBnUpdate()
{
    GetHost()->GetCurrGame()->UpdatePalData();

    bPalChanged = FALSE;
}

void CPalModDlg::OnButtonClickCheckEdits()
{
    GetHost()->GetCurrGame()->ValidateMixExtraColors(&fFileChanged);
}

void CPalModDlg::OnFilePatch()
{
    if (bPalChanged)
    {
        OnBnUpdate();
    }

    GetHost()->GetLoader()->SaveGame(GetHost()->GetCurrGame());

    SetStatusText(GetHost()->GetLoader()->GetLoadSaveStr());

    if (!GetHost()->GetLoader()->GetErrCt())
    {
        fFileChanged = FALSE;
    }
}

void CPalModDlg::OnGetCol()
{
    bGetCol = !bGetCol;

    CMenu* pSettMenu = GetMenu()->GetSubMenu(3); //3 = settings menu

    pSettMenu->CheckMenuItem(ID_GETCOLORONSELECT, MF_BYCOMMAND | (bGetCol ? MF_CHECKED : MF_UNCHECKED));

    //Enable/Disable get color button
    GetDlgItem(IDC_BNEWCOL)->EnableWindow(bGetCol && !bAutoSetCol);
}

void CPalModDlg::OnAutoSetCol()
{
    bAutoSetCol = !bAutoSetCol;

    CMenu* pSettMenu = GetMenu()->GetSubMenu(3); //3 = settings menu

    pSettMenu->CheckMenuItem(ID_AUTOSETCOL, MF_BYCOMMAND | (bAutoSetCol ? MF_CHECKED : MF_UNCHECKED));

    UpdateSliderSel();
}

void CPalModDlg::OnNMReleasedCaptureAll(NMHDR* pNMHDR, LRESULT* pResult)
{
    if (!bGetSliderUndo)
    {
        bGetSliderUndo = TRUE;
    }

    *pResult = 0;
}

void CPalModDlg::ProcChange(BOOL bReset)
{
    if (bReset)
    {
        UndoProc.Clear();

        bPalChanged = FALSE;
    }
    else
    {
        UndoProc.DeleteRedoList();
        NewUndoData();

        fFileChanged = TRUE;
        bPalChanged = TRUE;
    }
}

void CPalModDlg::OnFileExit()
{
    OnClose();
}

void CPalModDlg::OnFileCloseFileDir()
{
    if (VerifyMsg(VM_FILECHANGE))
    {
        CloseFileDir();
    }
}

void CPalModDlg::OnBnShowPrev()
{
    CPreviewDlg* PreviewDlg = GetHost()->GetPreviewDlg();
    if (!PreviewDlg->IsWindowVisible())
    {
        PreviewDlg->ShowWindow(SW_SHOW);

        PreviewDlg->m_ImgDisp.UpdateCtrl();
    }
}

void CPalModDlg::LoadLastDir()
{
    int nLastUsedGFlag;
    BOOL bIsDir;
    TCHAR szLastDir[MAX_PATH];

    if (GetLastUsedDirectory(szLastDir, sizeof(szLastDir), &nLastUsedGFlag, FALSE, &bIsDir))
    {
        if (VerifyMsg(VM_FILECHANGE))
        {
            if (nLastUsedGFlag > NUM_GAMES || nLastUsedGFlag < 0)
            {
                CString strError;
                strError.LoadString(IDS_ERROR_PARAMETERS);
                MessageBox(strError, GetAppName(), MB_ICONERROR);
                return;
            }
            else
            {
                if (bIsDir)
                {
                    LoadGameDir(nLastUsedGFlag, szLastDir);
                }
                else
                {
                    LoadGameFile(nLastUsedGFlag, szLastDir);
                }
            }
        }
    }
    else
    {
        SetStatusText(CString("Could not load previous file or directory"));
    }
}

int CALLBACK OnBrowseDialog(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
    {
        TCHAR szPath[MAX_PATH];

        if (GetLastUsedDirectory(szPath, sizeof(szPath), NULL))
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)szPath);
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

void SetLastUsedDirectory(LPCTSTR ptszPath, int nGameFlag)
{
    if (NULL != ptszPath)
    {
        HKEY hKey = NULL;

        //Set the directory / Game Flag
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_AppRegistryRoot, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_SET_VALUE, NULL, &hKey, NULL))
        {
            RegSetValueEx(hKey, c_strLastUsedPath, 0, REG_SZ, (LPBYTE)ptszPath, (DWORD)(_tcslen(ptszPath) + 1) * sizeof(TCHAR));
            RegSetValueEx(hKey, c_strLastUsedGFlag, 0, REG_DWORD, (LPBYTE)&nGameFlag, (DWORD)sizeof(int));

            RegCloseKey(hKey);
        }
    }

    return;
}

BOOL GetLastUsedDirectory(LPTSTR ptszPath, DWORD cbSize, int* nGameFlag, BOOL bCheck, BOOL* bIsDir)
{
    BOOL fFound = FALSE;
    HKEY hKey = NULL;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_AppRegistryRoot, 0, KEY_QUERY_VALUE, &hKey))
    {
        DWORD dwRegType = REG_SZ;
        TCHAR szPath[MAX_PATH];
        DWORD cbDataSize = sizeof(szPath);

        //Get the directory
        if ((ERROR_SUCCESS == RegQueryValueEx(hKey, c_strLastUsedPath, 0, &dwRegType, (LPBYTE)szPath, &cbDataSize))
            && (REG_SZ == dwRegType))
        {
            if (bCheck)
            {
                fFound = TRUE;
            }
            else
            {
                DWORD dwAttribs = GetFileAttributes(szPath);

                if ((INVALID_FILE_ATTRIBUTES != dwAttribs) && ((dwAttribs & FILE_ATTRIBUTE_DIRECTORY) || (dwAttribs & FILE_ATTRIBUTE_ARCHIVE)))
                {
                    if (bIsDir)
                    {
                        //Check to see if it's actually a file without an extension
                        *bIsDir = (dwAttribs & FILE_ATTRIBUTE_DIRECTORY);
                    }

                    strcpy(ptszPath, szPath);
                    fFound = TRUE;
                }
            }
        }

        //Grab the game flag
        if (nGameFlag)
        {
            nGameFlag ? *nGameFlag = 0xFF : 0;

            dwRegType = REG_DWORD;
            cbDataSize = sizeof(int);

            if ((ERROR_SUCCESS == RegQueryValueEx(hKey, c_strLastUsedGFlag, 0, &dwRegType, (LPBYTE)nGameFlag, &cbDataSize)))
            {
                //fFound = TRUE;
            }
        }

        RegCloseKey(hKey);
    }

    return(fFound);
}

void CPalModDlg::OnSetFocus(CWnd* pOldWnd)
{
    CDialog::OnSetFocus(pOldWnd);

    // TODO: Add your message handler code here
}

void CPalModDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CDialog::OnActivate(nState, pWndOther, bMinimized);

    // TODO: Add your message handler code here
}

void CPalModDlg::OnFileOpen()
{
    CString szGameFileDef = "";

    // BUGBUG... maybe remember their last selection?
    szGameFileDef.Append("SFIII3 51 Rom|51|"); //SFIII3
    szGameFileDef.Append("SSF2T sfxe.04a (*.04a)|*.04a|"); //SSF2T
    szGameFileDef.Append("SFA3 sz3.09c (*.09c )|*.09c|"); //SSF2T
    szGameFileDef.Append("XMVSF xvs.05a (*.05a )|*.05a|"); //XMVSF
    szGameFileDef.Append("MVC mvc.06 (*.06 )|*.06|"); //MVC
    szGameFileDef.Append("Jojos Roms|50; 51|"); //Jojos

    szGameFileDef.Append("|"); //End

    CFileDialog OpenDialog(
        TRUE,
        NULL,
        NULL,
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        szGameFileDef
    );

    if (OpenDialog.DoModal() == IDOK)
    {
        OPENFILENAME ofn = OpenDialog.GetOFN();
        switch (ofn.nFilterIndex)
        {
        case 1:
            LoadGameFile(SFIII3_A, (CHAR*)ofn.lpstrFile);
            break;
        case 2:
            LoadGameFile(SSF2T_A, (CHAR*)ofn.lpstrFile);
            break;
        case 3:
            LoadGameFile(SFA3_A, (CHAR*)ofn.lpstrFile);
            break;
        case 4:
            LoadGameFile(XMVSF_A, (CHAR*)ofn.lpstrFile);
            break;
        case 5:
            LoadGameFile(MVC_A, (CHAR*)ofn.lpstrFile);
            break;
        case 6:
            LoadGameFile(JOJOS_A, (CHAR*)ofn.lpstrFile);
            break;
        default:
            OutputDebugString("Error: game file not handled yet.\n");
            break;
        }
    }
}

void CPalModDlg::LoadGameFile(int nGameFlag, CHAR* szFile)
{
    if (!VerifyMsg(VM_FILECHANGE))
    {
        return;
    }

    CGameClass* GameGet = GetHost()->GetLoader()->LoadFile(nGameFlag, szFile);//szGet.GetBuffer());

    if (GameGet)
    {
        ClearGameVar();
        GetHost()->SetGameClass(GameGet);

        //Set the last used location
        SetLastUsedDirectory(szFile, GetHost()->GetCurrGame()->GetGameFlag());

        //The game has loaded OK
        PostGameLoad();
    }
    else
    {
        CString strError;
        strError.LoadString(IDS_ERROR_LOADING_GAME);
        MessageBox(strError, GetAppName(), MB_ICONERROR);
    }

    //Since we loaded a DIR, show status of load
    SetStatusText(GetHost()->GetLoader()->GetLoadSaveStr());
}

void CPalModDlg::OnBnBlink()
{
    Blink();
}

void CPalModDlg::OnLoadAct()
{
    if (bEnabled)
    {
        CFileDialog ActLoad(TRUE, NULL, NULL, NULL, "ACT Palette (*.ACT)| *.ACT||");

        if (ActLoad.DoModal() == IDOK)
        {
            CFile ActFile;

            if (ActFile.Open(ActLoad.GetOFN().lpstrFile, CFile::modeRead | CFile::typeBinary))
            {
                ProcChange();

                UINT8* pPal = (UINT8*)CurrPalCtrl->GetBasePal();
                int nWorkingAmt = CurrPalCtrl->GetWorkingAmt();
                int nFileSz = (int)ActFile.GetLength();

                if (nWorkingAmt * 3 > nFileSz)
                {
                    nWorkingAmt = nFileSz / 3;
                }

                UINT8* pAct = new UINT8[nWorkingAmt * 3];
                memset(pAct, 0, nWorkingAmt * 3);

                ActFile.Read(pAct, nWorkingAmt * 3);
                ActFile.Close();

                for (int i = 0; i < nWorkingAmt; i++)
                {
                    pPal[i * 4] = MainPalGroup->ROUND_R(pAct[i * 3]);
                    pPal[i * 4 + 1] = MainPalGroup->ROUND_G(pAct[i * 3 + 1]);
                    pPal[i * 4 + 2] = MainPalGroup->ROUND_B(pAct[i * 3 + 2]);

                    CurrPalCtrl->UpdateIndex(i);
                }

                ImgDispCtrl->UpdateCtrl();
                CurrPalCtrl->UpdateCtrl();

                delete[] pAct;

                SetStatusText(CString("Act file Loaded succesfully!"));
            }
            else
            {
                CString strError;
                strError.LoadString(IDS_ERROR_LOADING_ACT_FILE);
                MessageBox(strError, GetAppName(), MB_ICONERROR);
            }
        }
    }
}

void CPalModDlg::OnSaveAct()
{
    CFileDialog ActSave(FALSE, ".act", NULL, 4 | 2, "ACT Palette (*.ACT)| *.act|| All Files (*.*)| *.*||");

    if (ActSave.DoModal() == IDOK)
    {
        CFile ActFile;

        CString szFile = ActSave.GetOFN().lpstrFile;

        if (ActFile.Open(ActSave.GetOFN().lpstrFile, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
        {
            // We are writing this file in accordance with the spec as found here--
            //   https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577411_pgfId-1070626
            // In theory we should be able to just write a 768 byte file, but there appears to be a bug in PhotoShop's
            // ACT import wherein they mangle the parse for 768b files.  Thus we are forcibly using 772b here.

            int nActSz = 256 * 3;
            UINT8* pAct = new UINT8[nActSz];
            UINT8* pPal = (UINT8*)CurrPalCtrl->GetBasePal();
            int nWorkingAmt = CurrPalCtrl->GetWorkingAmt();

            memset(pAct, 0, nActSz);

            for (int i = 0; i < nWorkingAmt; i++)
            {
                pAct[i * 3] = pPal[i * 4];
                pAct[i * 3 + 1] = pPal[i * 4 + 1];
                pAct[i * 3 + 2] = pPal[i * 4 + 2];
            }

            ActFile.Write(pAct, nActSz);

            // Add 4 bytes per the 772b file syntax...
            DWORD finalWord;
            // HIWORD here is the number of useful colors in the file.
            finalWord = nWorkingAmt << 8;
            // LOWORD here is be the index to use for the transparency color.  This is 0 in all the games we care about.
            ActFile.Write(&finalWord, 4);

            ActFile.Close();

            delete[] pAct;

            SetStatusText(CString("Act file saved succesfully!"));
        }
        else
        {
            CString strError;
            strError.LoadString(IDS_ERROR_SAVING_ACT_FILE);
            MessageBox(strError, GetAppName(), MB_ICONERROR);
        }
    }
}

void CPalModDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    // TODO: Add your message handler code here and/or call default

    if (bCanMinMax)
    {
        //lpMMI->ptMinTrackSize = r
    }

    CDialog::OnGetMinMaxInfo(lpMMI);
}

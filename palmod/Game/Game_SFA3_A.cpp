#include "StdAfx.h"
#include "Game_SFA3_A.h"
#include "GameDef.h"

CDescTree CGame_SFA3_A::MainDescTree = CGame_SFA3_A::InitDescTree();

CGame_SFA3_A::CGame_SFA3_A(void)
{
    //We need the proper unit amt before we init the main buffer
    nUnitAmt = SFA3_A_NUMUNIT;

    InitDataBuffer();

    //Set color mode
    SetColMode(COLMODE_12A);

    //Set palette conversion mode
    BasePalGroup.SetMode(PALTYPE_17);

    //Set game information
    nGameFlag = SFA3_A;
    nImgGameFlag = IMG4;
    nImgUnitAmt = nUnitAmt;

    nDisplayW = 8;
    nFileAmt = 1;

    //Set the image out display type
    DisplayType = DISP_DEF;
    pButtonLabel = const_cast<CHAR*>((CHAR*)DEF_BUTTONLABEL6);

    //Create the redirect buffer
    rgUnitRedir = new UINT16[nUnitAmt + 1];
    memset(rgUnitRedir, NULL, sizeof(UINT16) * nUnitAmt);

    //Create the file changed flag
    rgFileChanged = new UINT16;

    nRGBIndexAmt = 15;
    nAIndexAmt = 0;

    nRGBIndexMul = 17.0f;
    nAIndexMul = 0.0f;
}

CGame_SFA3_A::~CGame_SFA3_A(void)
{
    ClearDataBuffer();
    //Get rid of the file changed flag
    safe_delete(rgFileChanged);
}

CDescTree* CGame_SFA3_A::GetMainTree()
{
    return &CGame_SFA3_A::MainDescTree;
}

CDescTree CGame_SFA3_A::InitDescTree()
{
    sDescTreeNode* NewDescTree = new sDescTreeNode;

    sDescTreeNode* UnitNode;
    sDescTreeNode* ButtonNode;
    sDescNode* ChildNode;

    //Create the main character tree
    sprintf(NewDescTree->szDesc, "%s", g_GameFriendlyName[SFA3_A]);
    NewDescTree->ChildNodes = new sDescTreeNode[SFA3_A_NUMUNIT];
    NewDescTree->uChildAmt = SFA3_A_NUMUNIT;
    //All units have tree children
    NewDescTree->uChildType = DESC_NODETYPE_TREE;

    //Go through each character
    for (int iUnitCtr = 0; iUnitCtr < SFA3_A_NUMUNIT; iUnitCtr++)
    {
        UnitNode = &((sDescTreeNode*)NewDescTree->ChildNodes)[iUnitCtr];
        //Set each description / character name
        sprintf(UnitNode->szDesc, "%s", SFA3_A_UNITDESC[iUnitCtr]);

        //Init each character to have all 6 basic buttons + extra
        UnitNode->ChildNodes = new sDescTreeNode[BUTTON6];

        //All children have button trees
        UnitNode->uChildType = DESC_NODETYPE_TREE;
        UnitNode->uChildAmt = BUTTON6;

        for (int iButtonCtr = 0; iButtonCtr < BUTTON6; iButtonCtr++)
        {
            int nCurrChildAmt = 1; // 1 for each button for now

            ButtonNode = &((sDescTreeNode*)UnitNode->ChildNodes)[iButtonCtr];

            //Set each button data
            sprintf(ButtonNode->szDesc, "%s", DEF_BUTTONLABEL6_SFA3[iButtonCtr]);

            //Button children have nodes
            ButtonNode->uChildType = DESC_NODETYPE_NODE;
            ButtonNode->uChildAmt = nCurrChildAmt;

            ButtonNode->ChildNodes = (sDescTreeNode*)new sDescNode[nCurrChildAmt];

            //Set each button's node
            ///////////////////////////////////////////////////////////////////////

            ChildNode = &((sDescNode*)ButtonNode->ChildNodes)[0]; //We only have 1

            sprintf(ChildNode->szDesc, "%s", DEF_BUTTONLABEL6_SFA3[iButtonCtr]);

            ChildNode->uUnitId = iUnitCtr;
            ChildNode->uPalId = iButtonCtr;
        }
    }

    return CDescTree(NewDescTree);
}

sFileRule CGame_SFA3_A::GetRule(UINT16 nUnitId)
{
    sFileRule NewFileRule;

    sprintf_s(NewFileRule.szFileName, MAX_FILENAME_LENGTH, "sz3.09c");

    NewFileRule.uUnitId = 0;
    NewFileRule.uVerifyVar = 0x80000;

    return NewFileRule;
}

int CGame_SFA3_A::GetBasicAmt(UINT16 nUnitId)
{
    return 6; // 6 for the 6 Ism-based colors
}

void CGame_SFA3_A::InitDataBuffer()
{
    pppDataBuffer = new UINT16 * *[nUnitAmt];
    memset(pppDataBuffer, NULL, sizeof(UINT16**) * nUnitAmt);
}

void CGame_SFA3_A::ClearDataBuffer()
{
    if (pppDataBuffer)
    {
        for (int nUnitCtr = 0; nUnitCtr < nUnitAmt; nUnitCtr++)
        {
            if (pppDataBuffer[nUnitCtr])
            {
                int nPalAmt = GetPalCt(nUnitCtr);

                for (int nPalCtr = 0; nPalCtr < nPalAmt; nPalCtr++)
                {
                    safe_delete_array(pppDataBuffer[nUnitCtr][nPalCtr]);
                }

                safe_delete_array(pppDataBuffer[nUnitCtr]);
            }
        }

        safe_delete_array(pppDataBuffer);
    }
}

void CGame_SFA3_A::GetPalOffsSz(UINT16 nUnitId, UINT16 nPalId)
{
    nCurrPalOffs = 0x2c0d4 + (0x3C0 * nUnitId) + (nPalId * 0xA0);
    nCurrPalSz = 16;
}

BOOL CGame_SFA3_A::LoadFile(CFile* LoadedFile, UINT16 nUnitId)
{
    for (UINT16 nUnitCtr = 0; nUnitCtr < nUnitAmt; nUnitCtr++)
    {
        UINT16 nPalAmt = GetPalCt(nUnitCtr);

        pppDataBuffer[nUnitCtr] = new UINT16 * [nPalAmt];

        rgUnitRedir[nUnitCtr] = nUnitCtr; //Fix later for unit sort

        for (UINT16 nPalCtr = 0; nPalCtr < nPalAmt; nPalCtr++)
        {
            GetPalOffsSz(nUnitCtr, nPalCtr);

            pppDataBuffer[nUnitCtr][nPalCtr] = new UINT16[nCurrPalSz];

            LoadedFile->Seek(nCurrPalOffs, CFile::begin);

            LoadedFile->Read(pppDataBuffer[nUnitCtr][nPalCtr], nCurrPalSz * 2);
        }
    }

    rgUnitRedir[nUnitAmt] = INVALID_UNIT_VALUE;

    return TRUE;
}

int CGame_SFA3_A::GetPalCt(UINT16 nUnitId)
{
    return 6;// 6 palettes in a unit
}

BOOL CGame_SFA3_A::SaveFile(CFile* SaveFile, UINT16 nUnitId)
{
    for (int nUnitCtr = 0; nUnitCtr < nUnitAmt; nUnitCtr++)
    {
        int nPalAmt = GetPalCt(nUnitCtr);

        for (int nPalCtr = 0; nPalCtr < nPalAmt; nPalCtr++)
        {
            GetPalOffsSz(nUnitCtr, nPalCtr);

            SaveFile->Seek(nCurrPalOffs, CFile::begin);

            SaveFile->Write(pppDataBuffer[nUnitCtr][nPalCtr], nCurrPalSz * 2);
        }
    }

    return TRUE;
}

void CGame_SFA3_A::CreateDefPal(sDescNode* srcNode, UINT16 nSepId)
{
    UINT16 nUnitId = srcNode->uUnitId;
    UINT16 nPalId = srcNode->uPalId;

    GetPalOffsSz(nUnitId, nPalId);

    BasePalGroup.AddPal(CreatePal(nUnitId, nPalId), nCurrPalSz, nUnitId, nPalId);
    BasePalGroup.AddSep(nSepId, srcNode->szDesc, 0, nCurrPalSz);
}

BOOL CGame_SFA3_A::UpdatePalImg(int Node01, int Node02, int Node03, int Node04)
{
    //Reset palette sources
    ClearSrcPal();

    if (Node01 == -1)
    {
        return FALSE;
    }

    sDescNode* NodeGet = MainDescTree.GetDescNode(Node01, Node02, Node03, Node04);

    if (NodeGet == NULL)
    {
        return FALSE;
    }

    UINT16 uUnitId = NodeGet->uUnitId;
    UINT16 uPalId = NodeGet->uPalId;

    // Make sure to reset the image id (currently not relevant, just sanity at this point)
    nTargetImgId = 0;
    int nImgUnitId = SFA3_A_IMGREDIR[uUnitId];

    int nSrcStart = 0;
    int nSrcAmt = GetBasicAmt(uUnitId);

    //Get rid of any palettes if there are any
    BasePalGroup.FlushPalAll();

    //Create the default palette
    ClearSetImgTicket(CreateImgTicket(nImgUnitId, nTargetImgId));

    CreateDefPal(NodeGet, 0);

    SetSourcePal(0, uUnitId, nSrcStart, nSrcAmt, 1);

    return TRUE;
}

COLORREF* CGame_SFA3_A::CreatePal(UINT16 nUnitId, UINT16 nPalId)
{
    GetPalOffsSz(nUnitId, nPalId);

    int nOffs = 0;

    COLORREF* NewPal = new COLORREF[nCurrPalSz];

    if (1 && nCurrPalSz > 1)//Fix later for extra palettes
    {
        nOffs = 1;
    }

    for (int i = nOffs; i < nCurrPalSz; i++)
    {
        NewPal[i] = ConvPal(pppDataBuffer[nUnitId][nPalId][i - nOffs]) | 0xFF000000;
    }

    if (nOffs)
    {
        NewPal[0] = 0xFF000000;
    }

    return NewPal;
}

void CGame_SFA3_A::UpdatePalData()
{
    for (int nPalCtr = 0; nPalCtr < MAX_PAL; nPalCtr++)
    {
        sPalDef* srcDef = BasePalGroup.GetPalDef(nPalCtr);

        if (srcDef->bAvail)
        {
            int nIndexStart = 1;

            COLORREF* crSrc = srcDef->pPal;
            UINT16 uAmt = srcDef->uPalSz;

            for (int nPICtr = nIndexStart; nPICtr < uAmt; nPICtr++)
            {
                pppDataBuffer[srcDef->uUnitId][srcDef->uPalId][nPICtr - 1] = (ConvCol(crSrc[nPICtr]) & 0x0FFF);
            }

            srcDef->bChanged = FALSE;
            rgFileChanged[0] = TRUE;
        }
    }
}

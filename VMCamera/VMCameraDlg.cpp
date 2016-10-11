// VMCameraDlg.cpp : implementation file
//

#include "stdafx.h"
#include "VMCamera.h"
#include "VMCameraDlg.h"
#include "SYSocket.h"
#include "SYProcess.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

SYTCPSocket *g_pServerSocket = NULL;
CVMCameraDlg *g_pThis = NULL;
SYProcess *g_syProcess = NULL;
void OnSYTCPSocketEvent(SYTCPSocket *sender, SYTCPEvent e);



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CVMCameraDlg dialog




CVMCameraDlg::CVMCameraDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVMCameraDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVMCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVMCameraDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CVMCameraDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CVMCameraDlg message handlers

BOOL CVMCameraDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	g_pThis = this;
	g_syProcess = new SYProcess();
	_listBox1 = (CListBox*)GetDlgItem(IDC_LIST1);
	//clear listbox	
	_listBox1->SetCurSel(-1);
	_listBox1->ResetContent();

	g_pServerSocket = new SYTCPSocket();
	g_pServerSocket->OnEvent  = OnSYTCPSocketEvent;
	g_pServerSocket->LocalPort = 6000;
	g_pServerSocket->Listen();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVMCameraDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVMCameraDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVMCameraDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
//======================================================================================================================
//======================================================================================================================
//////////////////////////////////////////////////////////////////////////
void OnSYTCPSocketEvent(SYTCPSocket *sender, SYTCPEvent e)
{
	switch (e.Status)
	{
	case SYTCPSOCKET_CLOSE:
		OutputDebugString(L"SYTCPSOCKET_CLOSE\n");
		break;

	case SYTCPSOCKET_RECVDATA:
		OutputDebugString(L"SYTCPSOCKET_RECVDATA\n");
		//OutputDebugStringA(e.szData);
		g_syProcess->RecvBuffer(e.szData, e.iLen);		
		break;

	case SYTCPSOCKET_CONNECTED:
		OutputDebugString(L"SYTCPSOCKET_CONNECTED\n");
		g_pThis->_listBox1->AddString(L"...Join us\n");
		g_syProcess->Clear();
		break;

	case SYTCPSOCKET_CONNECTFAULT:
		OutputDebugString(L"SYTCPSOCKET_CONNECTFAULT\n");
		break;

	case SYTCPSOCKET_DISCONNECT:
		OutputDebugString(L"SYTCPSOCKET_DISCONNECT\n");

		
		break;

	case SYTCPSOCKET_LISTENED:
		OutputDebugString(L"SYTCPSOCKET_LISTENED\n");		
		g_pThis->_listBox1->AddString(L"Listen.....Port:6000\n");

		break;
	}
}
//======================================================================================================================

void CVMCameraDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}

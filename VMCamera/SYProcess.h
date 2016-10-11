#include <shlwapi.h>
#include <string>
#include <fstream>//for load file

class SYFileData
{
public:	
	int   curIndex; //1 2 3 4 5
	int   maxCount;	//5
	int	  iDataLen; 
	int	  iWidth;
	int   iHeight;
	char  szData[65535];  //ProcSendFileThread need fix buffer_len	
};


class SYProcess
{
public:
	SYProcess()
	{
		_pszBuffer = NULL;
		//_outfile = 0;

		memset(_wszSaveRootPath, 0, sizeof(_wszSaveRootPath));
		GetModuleFileName(NULL, _wszSaveRootPath, sizeof(_wszSaveRootPath));
		PathRemoveFileSpec(_wszSaveRootPath);	
		wsprintf(_wszSaveRootPath, L"%s\\image\\", _wszSaveRootPath);
		OutputDebugString(_wszSaveRootPath);
	}

	~SYProcess()
	{
		free(_pszBuffer);
		if (_outfile)
		{
			_outfile.close();
			
		}
	}

	void RecvBuffer(char *pData, int Len)
	{
		if (_pszBuffer == NULL)
		{	
			_pszBuffer = (char*)malloc(sizeof(char)*Len);
			memset(_pszBuffer, 0, sizeof(char)*Len);
			memcpy( _pszBuffer, pData, sizeof(char)*Len);		
			_bufferLen = Len;
		}else{
			int size = _bufferLen;
			_pszBuffer = (char*)realloc(_pszBuffer, sizeof(char) * (size+Len));

			memset(_pszBuffer+size, 0, sizeof(char)*Len);
			memcpy(_pszBuffer+size, pData, sizeof(char)*Len);

			_bufferLen+=Len;
		}	

		ProcessBuffer();
	}

	
	void RemoveBuffer(int size)
	{
		if (_bufferLen-size == 0){
			free(_pszBuffer);
			_pszBuffer = NULL;
			_bufferLen = 0;
			return;
		}

		//copy
		char *pszTmp = (char*)malloc(sizeof(char*)*(_bufferLen-size));
		memset(pszTmp, 0, sizeof(char)*(_bufferLen-size));
		memcpy(pszTmp, _pszBuffer+size, sizeof(char)*(_bufferLen-size));

		//resize
		_pszBuffer = (char*)realloc(_pszBuffer, sizeof(char) * (_bufferLen-size));

		//recopy
		memset(_pszBuffer, 0, sizeof(char) * (_bufferLen-size));
		memcpy(_pszBuffer, pszTmp, sizeof(char) * (_bufferLen-size));

		free(pszTmp);

		_bufferLen = (_bufferLen-size);
	}

	
	void ProcessBuffer()
	{
		if (_bufferLen >= sizeof(SYFileData))
		{
			SYFileData fileData;
			ZeroMemory(&fileData, sizeof(SYFileData));
			memcpy(&fileData, _pszBuffer, sizeof(SYFileData));
			RemoveBuffer(sizeof(SYFileData));
			OutputDebugString(L"SYBTSDPCMD_SERVER_RESP_GETLIST\n");				
			
			if (fileData.curIndex == 1)
			{

				SYSTEMTIME time;
				GetSystemTime(&time);
				WORD millis = (time.wSecond * 1000) + time.wMilliseconds;
				wchar_t wszbuf[512];
				memset(wszbuf, 0, sizeof(wszbuf));
				wsprintf(wszbuf, L"%s%d.jpg", _wszSaveRootPath, millis);
				_outfile.open(wszbuf, std::ofstream::binary);
				if(!_outfile) {
					OutputDebugString(L"Open fault!");
				}				
			}

			if(_outfile) {
				_outfile.write (fileData.szData, fileData.iDataLen);	
			}
			

			if (fileData.curIndex == fileData.maxCount)
			{

				_outfile.close();				
			}
			ProcessBuffer();
		}
	}

	void Clear()
	{		
		RemoveBuffer(_bufferLen);		
		
	}
private:
	wchar_t _wszSaveRootPath[512];
	char *_pszBuffer;
	int _bufferLen;
	std::ofstream _outfile;

};
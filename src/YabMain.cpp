#include <File.h>
#include <String.h>
#include <stdio.h>
#include "YabInterface.h"

char t[1024];
const char* readSignature(int argc, char** argv)
{
	BString tmp("application/x-vnd.yab-app");
	/* Do not make changes above this comment without changing yab-IDE
	 to compensate for these changes.*/ 	
	for(int i=1; i<argc; i++)
	{
		if(argv[i][0]!='-')
		{
			BFile file(argv[i], B_READ_ONLY);
			if(file.InitCheck()==B_OK)
			{
				char readData[1024];
				int pos;
				file.Read(readData,1024);
				BString tmpString(readData);
				pos = tmpString.IFindFirst("MIMETYPE");
				if(pos!=B_ERROR)
				{
					int quote1, quote2;
					quote1 = tmpString.FindFirst("\"",pos);
					if(quote1!=B_ERROR)
					{
						quote2 = tmpString.FindFirst("\"",quote1+1);
						if(quote2!=B_ERROR)
						{
							tmp.SetTo("");
							tmpString.CopyInto(tmp,quote1+1,quote2-quote1-1);
						}
					}
				}
			}
			break;
		}
	}
	strcpy(t,tmp.String());
	return (const char*)t;
}

int main(int argc, char** argv)
{
	int ret;
	YabInterface *yabInterface = new YabInterface(argc, argv, readSignature(argc, argv));
	yabInterface->Run();
	ret = yabInterface->GetErrorCode();
	delete yabInterface;
	return ret;
}

# ccsMeasure

usage:

	ccsMeasure -a [CCS]
		get token of [CCS] and write to file

	ccsMeasure -c [CCS] [FILESIZE]
		create file of [FILESIZE] bytes. file name = "/data[FILESIZE]"

	ccsMeasure -u [CCS] [FILESIZE]
		record time and result for uploading file of [FILESIZE] bytes

	ccsMeasure -d [CCS] [FILESIZE|old|share]
		record time and result for downloading file with name "/data[FILESIZE|old|share]"

	[CCS] = dropbox|onedrive|googledrive|baidupcs


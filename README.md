# teletext-server
Teletext data server

# Todo

- Write a better README.md
- Write a better TODO list
- Implement subpage support
- Extensive(!!) refactoring
- Better portability (currently winsock only)

# Requirements:

- boost 1.57 (sorry)

Edit Protocol:

	Data types:

	<page-address>:	Identifies a page. 
		Current format: [1-8][0-F][0-F].
		In the future this identifier will also include any subpage.

	<line-num>: Two ascii-hex characters specifying the line number to be updated

	<page data>: 80 characters with 40 bytes of ASCII-hex-encoded ("00" thruogh "FF") teletext data (without parity)
		Current page format: 25*80 bytes.
	
	Currently implemented commands:
	
	LOGIN <user-name> <password>

	UPDATE <page-address> <line-num> <page data>

	BYE

	Possible future commands: 

	LIST PAGES
		Requests a list of pages

	FLASH <page-address>
		Requests that the carousel diverges from its normal page sequence, and
		immediately send the given page.

	GRANT <user> <page-address>

	REVOKE <user> <page-address>

	LIST USERS

	CREATE USER

	SHOW <page-address>

	HIDE <page-address>
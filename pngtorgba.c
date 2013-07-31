#include <stdio.h>
#include <sys/param.h>

main(int argc, char ** argv) {
	int i;
	FILE * rgb, * alpha;
	int width, height, awidth, aheight;
	static char buf[MAXPATHLEN];
	if (argc != 2 || *argv[1] == '-') {
		fprintf(stderr, "Usage: pngtorgba file.png\n");
		exit(1);
	}
	sprintf(buf, "pngtopnm %s | pnmnoraw", argv[1]);
	rgb = popen(buf, "r");
	sprintf(buf, "pngtopnm -alpha %s | pnmnoraw", argv[1]);
	alpha = popen(buf, "r");
	if (!rgb || !alpha) {
		fprintf(stderr, "PBMplus package is required\n");
		exit(1);
	}

	if (2 != fscanf(rgb, "P3 %d %d %*d", &width, &height)) {
		fprintf(stderr, "Bad RGB stream\n");
		exit(1);
	}
	
	if (2 != fscanf(alpha, "P2 %d %d %*d", &awidth, &aheight) ||
		awidth != width || aheight != height) {
		fprintf(stderr, "Bad alpha stream\n");
		exit(1);
	}

	printf("unsigned bk_icon_width = %d,\n\tbk_icon_height = %d,\n\tbk_icon[] = {\n", width, height);
	for (i =0; i < width*height; i++) {
		int r, g, b, a;
		fscanf(rgb, "%d %d %d", &r, &g, &b);
		fscanf(alpha, "%d", &a);
		printf("%#x%s", r<<24|g<<16|b<<8|a, i!=48*48-1?", ": "");
		if (i % 8 == 7) putchar('\n');
	} 
	printf("};\n");
	exit(0);
}

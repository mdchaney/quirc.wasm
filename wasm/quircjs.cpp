/*
 * Pulled from :
 * https://github.com/zz85/quirc.js
 *
 * @author zz85 github.com/zz85
 *
 * This is a simple wrapper to bring quirc
 * https://github.com/dlbeer/quirc
 * functionality to the browser via wasm.  zz85's fork had fallen
 * behind the dlbeer original by many commits, and zz85's changes
 * were minimal (but really cool) so I forked the original and
 * added zz85's changes here.  There are other good forks with the
 * appropriate configs for npm and such.
 */
extern "C" {
	#include <emscripten.h>
	#include <quirc.h>
	#include <stdio.h>
	#include <stdlib.h>

	struct quirc *qr;
	uint8_t *image;

	void setup() {
		qr = quirc_new();
		if (!qr) {
			perror("Failed to allocate memory");
		}
	}

	void teardown() {
		quirc_destroy(qr);
	}

	void resize(int width, int height) {
		 if (quirc_resize(qr, width, height) < 0) {
			perror("Failed to allocate video memory");
		}
	}

	void fill() {
		image = quirc_begin(qr, NULL, NULL);
	}

	void filled() {
		quirc_end(qr);
	}

	void process() {
		int num_codes;
		int i;

		num_codes = quirc_count(qr);
		EM_ASM_({
			if (self.counted) counted($0);
		}, num_codes);

		for (i = 0; i < num_codes; i++) {
			struct quirc_code code;
			struct quirc_data data;
			quirc_decode_error_t err;

			quirc_extract(qr, i, &code);

			/* Decoding stage */
			err = quirc_decode(&code, &data);
			if (err) {
				//printf("DECODE FAILED: %s\n", quirc_strerror(err));
				EM_ASM_({
					if (self.decode_error) {
						decode_error(UTF8ToString($0));
					}
				}, quirc_strerror(err));
			} else {
				// printf("Data: %s\n", data.payload);
				EM_ASM_({
					var a = arguments;
					var $i = 0;
					if (self.decoded) decoded(
						a[$i++], // i
						a[$i++], // version
						a[$i++], // ecc
						a[$i++], // mask
						a[$i++], // data type
						a[$i++], // payload
						a[$i++], // payload len
						a[$i++], // corners
						a[$i++],
						a[$i++],
						a[$i++],
						a[$i++],
						a[$i++],
						a[$i++],
						a[$i++]
					);
				},
					i,
					data.version,
					data.ecc_level,
					data.mask,
					data.data_type,
					data.payload,
					data.payload_len,
					code.corners[0].x,
					code.corners[0].y,
					code.corners[1].x,
					code.corners[1].y,
					code.corners[2].x,
					code.corners[2].y,
					code.corners[3].x,
					code.corners[3].y
				);
			}
		}
	}

	long xsetup(int width, int height) {
		setup();
		resize(width, height);
		fill();

		// return pointer to image
		return (long) image;
	}

	long xprocess() {
		filled();
		process();
		fill();
		return (long) image;
	}
}

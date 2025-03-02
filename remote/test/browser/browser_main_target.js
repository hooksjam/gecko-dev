/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

// Test very basic CDP features.

add_task(async function() {
  // Start the CDP server
  await RemoteAgent.listen(Services.io.newURI("http://localhost:9222"));

  const { mainProcessTarget } = RemoteAgent.targets;
  ok(
    mainProcessTarget,
    "The main process target is instantiated after the call to `listen`"
  );

  const targetURL = mainProcessTarget.wsDebuggerURL;

  const CDP = await getCDP();

  const client = await CDP({ target: targetURL });
  ok(true, "CDP client has been instantiated");

  const { Browser, Target } = client;
  ok(Browser, "The main process target exposes Browser domain");
  ok(Target, "The main process target exposes Target domain");

  const version = await Browser.getVersion();

  const { isHeadless } = Cc["@mozilla.org/gfx/info;1"].getService(
    Ci.nsIGfxInfo
  );
  const expectedProduct = isHeadless ? "Headless Firefox" : "Firefox";
  is(version.product, expectedProduct, "Browser.getVersion works");

  const { webSocketDebuggerUrl } = await CDP.Version();
  is(
    webSocketDebuggerUrl,
    targetURL,
    "Version endpoint refers to the same Main process target"
  );

  await RemoteAgent.close();
});

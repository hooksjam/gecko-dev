/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

var EXPORTED_SYMBOLS = ["HomePage"];

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);

XPCOMUtils.defineLazyModuleGetters(this, {
  IgnoreLists: "resource://gre/modules/IgnoreLists.jsm",
  PrivateBrowsingUtils: "resource://gre/modules/PrivateBrowsingUtils.jsm",
  Services: "resource://gre/modules/Services.jsm",
});

const kPrefName = "browser.startup.homepage";
const kDefaultHomePage = "about:home";
const kExtensionControllerPref =
  "browser.startup.homepage_override.extensionControlled";
const kHomePageIgnoreListId = "homepage-urls";

function getHomepagePref(useDefault) {
  let homePage;
  let prefs = Services.prefs;
  if (useDefault) {
    prefs = prefs.getDefaultBranch(null);
  }
  try {
    // Historically, this was a localizable pref, but default Firefox builds
    // don't use this.
    // Distributions and local customizations might still use this, so let's
    // keep it.
    homePage = prefs.getComplexValue(kPrefName, Ci.nsIPrefLocalizedString).data;
  } catch (ex) {}

  if (!homePage) {
    homePage = prefs.getStringPref(kPrefName);
  }

  // Apparently at some point users ended up with blank home pages somehow.
  // If that happens, reset the pref and read it again.
  if (!homePage && !useDefault) {
    Services.prefs.clearUserPref(kPrefName);
    homePage = getHomepagePref(true);
  }

  return homePage;
}

/**
 * HomePage provides tools to keep try of the current homepage, and the
 * applications's default homepage. It includes tools to insure that certain
 * urls are ignored. As a result, all set/get requests for the homepage
 * preferences should be routed through here.
 */
let HomePage = {
  // This is an array of strings that should be matched against URLs to see
  // if they should be ignored or not.
  _ignoreList: [],

  // A promise that is set when initialization starts and resolved when it
  // completes.
  _initializationPromise: null,

  /**
   * Used to initialise the ignore lists. This may be called later than
   * the first call to get or set, which may cause a used to get an ignored
   * homepage, but this is deemed acceptable, as we'll correct it once
   * initialised.
   */
  async init() {
    if (this._initializationPromise) {
      await this._initializationPromise;
      return;
    }

    Services.telemetry.setEventRecordingEnabled("homepage", true);

    // Now we have the values, listen for future updates.
    this._ignoreListListener = this._handleIgnoreListUpdated.bind(this);

    this._initializationPromise = IgnoreLists.getAndSubscribe(
      this._ignoreListListener
    );

    const current = await this._initializationPromise;

    await this._handleIgnoreListUpdated({ data: { current } });
  },

  /**
   * Gets the homepage for the given window.
   *
   * @param {DOMWindow} [aWindow]
   *   The window associated with the get, used to check for private browsing
   *   mode. If not supplied, normal mode is assumed.
   * @returns {string}
   *   Returns the home page value, this could be a single url, or a `|`
   *   separated list of URLs.
   */
  get(aWindow) {
    let homePages = getHomepagePref();
    if (
      PrivateBrowsingUtils.permanentPrivateBrowsing ||
      (aWindow && PrivateBrowsingUtils.isWindowPrivate(aWindow))
    ) {
      // If an extension controls the setting and does not have private
      // browsing permission, use the default setting.
      let extensionControlled = Services.prefs.getBoolPref(
        kExtensionControllerPref,
        false
      );
      let privateAllowed = Services.prefs.getBoolPref(
        "browser.startup.homepage_override.privateAllowed",
        false
      );
      // There is a potential on upgrade that the prefs are not set yet, so we double check
      // for moz-extension.
      if (
        !privateAllowed &&
        (extensionControlled || homePages.includes("moz-extension://"))
      ) {
        return this.getDefault();
      }
    }

    return homePages;
  },

  /**
   * @returns {string}
   *   Returns the application default homepage.
   */
  getDefault() {
    return getHomepagePref(true);
  },

  /**
   * @returns {string}
   *   Returns the original application homepage URL (not from prefs).
   */
  getOriginalDefault() {
    return kDefaultHomePage;
  },

  /**
   * @returns {boolean}
   *   Returns true if the homepage has been changed.
   */
  get overridden() {
    return Services.prefs.prefHasUserValue(kPrefName);
  },

  /**
   * @returns {boolean}
   *   Returns true if the homepage preference is locked.
   */
  get locked() {
    return Services.prefs.prefIsLocked(kPrefName);
  },

  /**
   * @returns {boolean}
   *   Returns true if the current homepage is the application default.
   */
  get isDefault() {
    return HomePage.get() === kDefaultHomePage;
  },

  /**
   * Sets the homepage preference to a new page.
   *
   * @param {string} value
   *   The new value to set the preference to. This could be a single url, or a
   *   `|` separated list of URLs.
   */
  async set(value) {
    await this.init();

    if (await this.shouldIgnore(value)) {
      Cu.reportError(
        `Ignoring homepage setting for ${value} as it is on the ignore list.`
      );
      Services.telemetry.recordEvent(
        "homepage",
        "preference",
        "ignore",
        "set_blocked"
      );
      return false;
    }
    Services.prefs.setStringPref(kPrefName, value);
    return true;
  },

  /**
   * Sets the homepage preference to a new page. This is an synchronous version
   * that should only be used when we know the source is safe as it bypasses the
   * ignore list, e.g. when setting directly to about:blank or a value not
   * supplied externally.
   *
   * @param {string} value
   *   The new value to set the preference to. This could be a single url, or a
   *   `|` separated list of URLs.
   */
  safeSet(value) {
    Services.prefs.setStringPref(kPrefName, value);
  },

  /**
   * Clears the homepage preference if it is not the default. Note that for
   * policy/locking use, the default homepage might not be about:home after this.
   */
  clear() {
    Services.prefs.clearUserPref(kPrefName);
  },

  /**
   * Resets the homepage preference to be about:home.
   */
  reset() {
    Services.prefs.setStringPref(kPrefName, kDefaultHomePage);
  },

  /**
   * Determines if a url should be ignored according to the ignore list.
   *
   * @param {string} url
   *   A string that is the url or urls to be ignored.
   * @returns {boolean}
   *   True if the url should be ignored.
   */
  async shouldIgnore(url) {
    await this.init();

    const lowerURL = url.toLowerCase();
    return this._ignoreList.some(code => lowerURL.includes(code.toLowerCase()));
  },

  /**
   * Handles updates of the ignore list, checking the existing preference and
   * correcting it as necessary.
   *
   * @param {Object} eventData
   *   The event data as received from RemoteSettings.
   */
  async _handleIgnoreListUpdated({ data: { current } }) {
    for (const entry of current) {
      if (entry.id == kHomePageIgnoreListId) {
        this._ignoreList = [...entry.matches];
      }
    }

    // Only check if we're overridden as we assume the default value is fine,
    // or won't be changeable (e.g. enterprise policy).
    if (this.overridden) {
      let homePages = getHomepagePref().toLowerCase();
      if (
        this._ignoreList.some(code => homePages.includes(code.toLowerCase()))
      ) {
        this.clear();
        Services.prefs.clearUserPref(kExtensionControllerPref);
        Services.telemetry.recordEvent(
          "homepage",
          "preference",
          "ignore",
          "saved_reset"
        );
      }
    }
  },
};

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-people-page' is the settings page containing sign-in settings.
 */
Polymer({
  is: 'settings-people-page',

  behaviors: [
    settings.RouteObserverBehavior, I18nBehavior, WebUIListenerBehavior,
    // <if expr="chromeos">
    CrPngBehavior, LockStateBehavior,
    // </if>
  ],

  properties: {
    /**
     * Preferences state.
     */
    prefs: {
      type: Object,
      notify: true,
    },

    // <if expr="not chromeos">
    /**
     * This flag is used to conditionally show a set of new sign-in UIs to the
     * profiles that have been migrated to be consistent with the web sign-ins.
     * TODO(scottchen): In the future when all profiles are completely migrated,
     * this should be removed, and UIs hidden behind it should become default.
     * @private
     */
    diceEnabled_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('diceEnabled');
      },
    },
    // </if>

    /**
     * The current sync status, supplied by SyncBrowserProxy.
     * @type {?settings.SyncStatus}
     */
    syncStatus: Object,

    /**
     * The currently selected profile icon URL. May be a data URL.
     * @private
     */
    profileIconUrl_: String,

    /**
     * The current profile name.
     * @private
     */
    profileName_: String,

    /**
     * The profile deletion warning. The message indicates the number of
     * profile stats that will be deleted if a non-zero count for the profile
     * stats is returned from the browser.
     * @private
     */
    deleteProfileWarning_: String,

    /**
     * True if the profile deletion warning is visible.
     * @private
     */
    deleteProfileWarningVisible_: Boolean,

    /**
     * True if the checkbox to delete the profile has been checked.
     * @private
     */
    deleteProfile_: Boolean,

    // <if expr="not chromeos">
    /** @private */
    showImportDataDialog_: {
      type: Boolean,
      value: false,
    },
    // </if>

    /** @private */
    showDisconnectDialog_: Boolean,

    // <if expr="chromeos">
    /**
     * True if fingerprint settings should be displayed on this machine.
     * @private
     */
    fingerprintUnlockEnabled_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('fingerprintUnlockEnabled');
      },
      readOnly: true,
    },
    // </if>

    /** @private {!Map<string, string>} */
    focusConfig_: {
      type: Object,
      value: function() {
        const map = new Map();
        if (settings.routes.SYNC)
          map.set(settings.routes.SYNC.path, '#sync-status .subpage-arrow');
        // <if expr="not chromeos">
        if (settings.routes.MANAGE_PROFILE) {
          map.set(
              settings.routes.MANAGE_PROFILE.path,
              '#picture-subpage-trigger .subpage-arrow');
        }
        // </if>
        // <if expr="chromeos">
        if (settings.routes.CHANGE_PICTURE) {
          map.set(
              settings.routes.CHANGE_PICTURE.path,
              '#picture-subpage-trigger .subpage-arrow');
        }
        if (settings.routes.LOCK_SCREEN) {
          map.set(
              settings.routes.LOCK_SCREEN.path,
              '#lock-screen-subpage-trigger .subpage-arrow');
        }
        if (settings.routes.ACCOUNTS) {
          map.set(
              settings.routes.ACCOUNTS.path,
              '#manage-other-people-subpage-trigger .subpage-arrow');
        }
        // </if>
        return map;
      },
    },
  },

  /** @private {?settings.SyncBrowserProxy} */
  syncBrowserProxy_: null,

  /** @override */
  attached: function() {
    const profileInfoProxy = settings.ProfileInfoBrowserProxyImpl.getInstance();
    profileInfoProxy.getProfileInfo().then(this.handleProfileInfo_.bind(this));
    this.addWebUIListener(
        'profile-info-changed', this.handleProfileInfo_.bind(this));

    this.addWebUIListener(
        'profile-stats-count-ready', this.handleProfileStatsCount_.bind(this));

    this.syncBrowserProxy_ = settings.SyncBrowserProxyImpl.getInstance();
    this.syncBrowserProxy_.getSyncStatus().then(
        this.handleSyncStatus_.bind(this));
    this.addWebUIListener(
        'sync-status-changed', this.handleSyncStatus_.bind(this));
    // <if expr="not chromeos">
    this.addWebUIListener('sync-settings-saved', () => {
      /** @type {!CrToastElement} */ (this.$.toast).show();
    });
    // </if>
  },

  /** @protected */
  currentRouteChanged: function() {
    this.showImportDataDialog_ =
        settings.getCurrentRoute() == settings.routes.IMPORT_DATA;

    if (settings.getCurrentRoute() == settings.routes.SIGN_OUT) {
      // <if expr="not chromeos">
      settings.ProfileInfoBrowserProxyImpl.getInstance().getProfileStatsCount();
      // </if>
      // If the sync status has not been fetched yet, optimistically display
      // the disconnect dialog. There is another check when the sync status is
      // fetched. The dialog will be closed then the user is not signed in.
      if (this.syncStatus && !this.syncStatus.signedIn) {
        settings.navigateToPreviousRoute();
      } else {
        this.showDisconnectDialog_ = true;
        this.async(() => {
          this.$$('#disconnectDialog').showModal();
        });
      }
    } else if (this.showDisconnectDialog_) {
      this.$$('#disconnectDialog').close();
    }
  },

  // <if expr="chromeos">
  /** @private */
  getPasswordState_: function(hasPin, enableScreenLock) {
    if (!enableScreenLock)
      return this.i18n('lockScreenNone');
    if (hasPin)
      return this.i18n('lockScreenPinOrPassword');
    return this.i18n('lockScreenPasswordOnly');
  },
  // </if>

  /**
   * Handler for when the profile's icon and name is updated.
   * @private
   * @param {!settings.ProfileInfo} info
   */
  handleProfileInfo_: function(info) {
    this.profileName_ = info.name;
    /**
     * Extract first frame from image by creating a single frame PNG using
     * url as input if base64 encoded and potentially animated.
     */
    // <if expr="chromeos">
    if (info.iconUrl.startsWith('data:image/png;base64')) {
      this.profileIconUrl_ =
          CrPngBehavior.convertImageSequenceToPng([info.iconUrl]);
      return;
    }
    // </if>

    this.profileIconUrl_ = info.iconUrl;
  },

  /**
   * Handler for when the profile stats count is pushed from the browser.
   * @param {number} count
   * @private
   */
  handleProfileStatsCount_: function(count) {
    this.deleteProfileWarning_ = (count > 0) ?
        (count == 1) ? loadTimeData.getStringF(
                           'deleteProfileWarningWithCountsSingular',
                           this.syncStatus.signedInUsername) :
                       loadTimeData.getStringF(
                           'deleteProfileWarningWithCountsPlural', count,
                           this.syncStatus.signedInUsername) :
        loadTimeData.getStringF(
            'deleteProfileWarningWithoutCounts',
            this.syncStatus.signedInUsername);
  },

  /**
   * Handler for when the sync state is pushed from the browser.
   * @param {?settings.SyncStatus} syncStatus
   * @private
   */
  handleSyncStatus_: function(syncStatus) {
    if (!this.syncStatus && syncStatus && !syncStatus.signedIn)
      chrome.metricsPrivate.recordUserAction('Signin_Impression_FromSettings');

    if (!syncStatus.signedIn && this.showDisconnectDialog_)
      this.$$('#disconnectDialog').close();

    this.syncStatus = syncStatus;
  },

  /** @private */
  onProfileTap_: function() {
    // <if expr="chromeos">
    settings.navigateTo(settings.routes.CHANGE_PICTURE);
    // </if>
    // <if expr="not chromeos">
    settings.navigateTo(settings.routes.MANAGE_PROFILE);
    // </if>
  },

  /** @private */
  onSigninTap_: function() {
    this.syncBrowserProxy_.startSignIn();
  },

  /** @private */
  onDisconnectClosed_: function() {
    this.showDisconnectDialog_ = false;
    // <if expr="not chromeos">
    if (!this.diceEnabled_) {
      // If DICE-enabled, this button won't exist here.
      cr.ui.focusWithoutInk(assert(this.$$('#disconnectButton')));
    }
    // </if>

    // <if expr="chromeos">
    cr.ui.focusWithoutInk(assert(this.$$('#disconnectButton')));
    // </if>

    if (settings.getCurrentRoute() == settings.routes.SIGN_OUT)
      settings.navigateToPreviousRoute();
    this.fire('signout-dialog-closed');
  },

  /** @private */
  onDisconnectTap_: function() {
    settings.navigateTo(settings.routes.SIGN_OUT);
  },

  /** @private */
  onDisconnectCancel_: function() {
    this.$$('#disconnectDialog').close();
  },

  /** @private */
  onDisconnectConfirm_: function() {
    const deleteProfile = !!this.syncStatus.domain || this.deleteProfile_;
    // Trigger the sign out event after the navigateToPreviousRoute().
    // So that the navigation to the setting page could be finished before the
    // sign out if navigateToPreviousRoute() returns synchronously even the
    // browser is closed after the sign out. Otherwise, the navigation will be
    // finished during session restore if the browser is closed before the async
    // callback executed.
    listenOnce(this, 'signout-dialog-closed', () => {
      this.syncBrowserProxy_.signOut(deleteProfile);
    });

    this.$$('#disconnectDialog').close();
  },

  /** @private */
  onSyncTap_: function() {
    assert(this.syncStatus.signedIn);
    assert(this.syncStatus.syncSystemEnabled);

    if (!this.isSyncStatusActionable_(this.syncStatus))
      return;

    switch (this.syncStatus.statusAction) {
      case settings.StatusAction.REAUTHENTICATE:
        this.syncBrowserProxy_.startSignIn();
        break;
      case settings.StatusAction.SIGNOUT_AND_SIGNIN:
        // <if expr="chromeos">
        this.syncBrowserProxy_.attemptUserExit();
        // </if>
        // <if expr="not chromeos">
        if (this.syncStatus.domain)
          settings.navigateTo(settings.routes.SIGN_OUT);
        else {
          // Silently sign the user out without deleting their profile and
          // prompt them to sign back in.
          this.syncBrowserProxy_.signOut(false);
          this.syncBrowserProxy_.startSignIn();
        }
        // </if>
        break;
      case settings.StatusAction.UPGRADE_CLIENT:
        settings.navigateTo(settings.routes.ABOUT);
        break;
      case settings.StatusAction.ENTER_PASSPHRASE:
      case settings.StatusAction.CONFIRM_SYNC_SETTINGS:
      case settings.StatusAction.NO_ACTION:
      default:
        settings.navigateTo(settings.routes.SYNC);
    }
  },

  // <if expr="chromeos">
  /**
   * @param {!Event} e
   * @private
   */
  onConfigureLockTap_: function(e) {
    // Navigating to the lock screen will always open the password prompt
    // dialog, so prevent the end of the tap event to focus what is underneath
    // it, which takes focus from the dialog.
    e.preventDefault();
    settings.navigateTo(settings.routes.LOCK_SCREEN);
  },
  // </if>

  /** @private */
  onManageOtherPeople_: function() {
    // <if expr="not chromeos">
    this.syncBrowserProxy_.manageOtherPeople();
    // </if>
    // <if expr="chromeos">
    settings.navigateTo(settings.routes.ACCOUNTS);
    // </if>
  },

  // <if expr="not chromeos">
  /**
   * @private
   * @param {string} domain
   * @return {string}
   */
  getDomainHtml_: function(domain) {
    const innerSpan = '<span id="managed-by-domain-name">' + domain + '</span>';
    return loadTimeData.getStringF('domainManagedProfile', innerSpan);
  },

  /** @private */
  onImportDataTap_: function() {
    settings.navigateTo(settings.routes.IMPORT_DATA);
  },

  /** @private */
  onImportDataDialogClosed_: function() {
    settings.navigateToPreviousRoute();
    cr.ui.focusWithoutInk(assert(this.$.importDataDialogTrigger));
  },

  /**
   * @return {boolean}
   * @private
   */
  shouldShowSyncAccountControl_: function() {
    return !!this.diceEnabled_ && !!this.syncStatus.syncSystemEnabled &&
        !!this.syncStatus.signinAllowed;
  },
  // </if>

  /**
   * @private
   * @param {string} domain
   * @return {string}
   */
  getDisconnectExplanationHtml_: function(domain) {
    // <if expr="not chromeos">
    if (domain) {
      return loadTimeData.getStringF(
          'syncDisconnectManagedProfileExplanation',
          '<span id="managed-by-domain-name">' + domain + '</span>');
    }
    // </if>
    return loadTimeData.getString('syncDisconnectExplanation');
  },

  /**
   * @private
   * @param {?settings.SyncStatus} syncStatus
   * @return {boolean}
   */
  isAdvancedSyncSettingsVisible_: function(syncStatus) {
    return !!syncStatus && !!syncStatus.signedIn &&
        !!syncStatus.syncSystemEnabled;
  },

  /**
   * @private
   * @param {?settings.SyncStatus} syncStatus
   * @return {boolean} Whether an action can be taken with the sync status. sync
   *     status is actionable if sync is not managed and if there is a sync
   *     error, there is an action associated with it.
   */
  isSyncStatusActionable_: function(syncStatus) {
    return !!syncStatus && !syncStatus.managed &&
        (!syncStatus.hasError ||
         syncStatus.statusAction != settings.StatusAction.NO_ACTION);
  },

  /**
   * @private
   * @param {?settings.SyncStatus} syncStatus
   * @return {string}
   */
  getSyncIcon_: function(syncStatus) {
    if (!syncStatus)
      return '';

    let syncIcon = 'settings:sync';

    if (syncStatus.hasError)
      syncIcon = 'settings:sync-problem';

    // Override the icon to the disabled icon if sync is managed.
    if (syncStatus.managed)
      syncIcon = 'settings:sync-disabled';

    return syncIcon;
  },

  /**
   * @private
   * @param {?settings.SyncStatus} syncStatus
   * @return {string} The class name for the sync status text.
   */
  getSyncStatusTextClass_: function(syncStatus) {
    return (!!syncStatus && syncStatus.hasError) ? 'sync-error' : '';
  },

  /**
   * @param {string} iconUrl
   * @return {string} A CSS image-set for multiple scale factors.
   * @private
   */
  getIconImageSet_: function(iconUrl) {
    return cr.icon.getImage(iconUrl);
  },

  /**
   * @param {!settings.SyncStatus} syncStatus
   * @return {boolean} Whether to show the "Sign in to Chrome" button.
   * @private
   */
  showSignin_: function(syncStatus) {
    return !!syncStatus.signinAllowed && !syncStatus.signedIn;
  },
});

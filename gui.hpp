#pragma once

#ifdef __cplusplus
  extern "C" {
    VOID EVEAPI TastNotificationShow(
      IN PWSTR szMessage
      );
  };

  namespace Gui {
    HRESULT EVEAPI Init(
      IN HINSTANCE hInstance
      );

    HRESULT EVEAPI Run(VOID);
  }
#else
  VOID EVEAPI TastNotificationShow(
    IN PWSTR szMessage
    );
#endif

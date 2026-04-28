// This document was automatically generated.

document.getElementById("lang-menu").onchange = (event) => {
  const lang = event.target.value;

  try {
    localStorage.setItem("lang", lang);
  } catch {
  }

  const curLang = document.documentElement.lang;
  console.assert(curLang);

  const curPathname = window.location.pathname;
  if (curPathname != `/${curLang}` &&
      !curPathname.startsWith(`/${curLang}/`))
    return;

  window.location.pathname = curPathname.replace(
    `/${curLang}`, `/${lang}`);
};

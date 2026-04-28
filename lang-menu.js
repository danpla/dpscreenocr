// This document was automatically generated.

document.getElementById("lang-menu").onchange = (event) => {
  const select = event.target;

  try {
    localStorage.setItem(
        "lang", select[select.selectedIndex].lang);
  } catch {
  }

  window.location = select.value;
};

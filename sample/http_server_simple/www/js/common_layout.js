(function() {
  function markActiveNav(root, currentPath) {
    var links = root.querySelectorAll('.site-nav a[data-path]');
    for (var index = 0; index < links.length; index++) {
      var link = links[index];
      if (link.getAttribute('data-path') === currentPath) {
        link.classList.add('is-active');
      }
    }
  }

  function renderHeaderAndFooter() {
    var headerHost = document.getElementById('site-header');
    var footerHost = document.getElementById('site-footer');
    var currentYear = new Date().getFullYear();
    var currentPath = window.location.pathname;

    if (headerHost) {
      headerHost.className = 'site-header';
      headerHost.innerHTML =
        '<div class="site-header-inner">' +
          '<a class="site-brand" href="/index.html">simple http server</a>' +
          '<nav class="site-nav">' +
            '<a href="/index.html" data-path="/index.html">Top</a>' +
            '<a href="/wsock.html" data-path="/wsock.html">Room Sample</a>' +
            '<a href="/programing_document.html" data-path="/programing_document.html">Programming Doc</a>' +
          '</nav>' +
        '</div>';

      markActiveNav(headerHost, currentPath);
    }

    if (footerHost) {
      footerHost.className = 'site-footer';
      footerHost.innerHTML =
        '<div class="site-footer-inner">' +
          '<span>simple http server</span>' +
          '<span>© ' + currentYear + ' sample pages</span>' +
        '</div>';
    }
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', renderHeaderAndFooter);
  } else {
    renderHeaderAndFooter();
  }
})();

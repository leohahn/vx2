;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

((c++-mode
  (projectile-project-run-cmd . "cd build && ./dr")
  (projectile-project-compilation-cmd . "cd build && ninja")
  (helm-make-build-dir . "build/")))

function debounce(func, timeout = 250) {
  let timer;
  return (...args) => {
    clearTimeout(timer);
    timer = setTimeout(() => {
      func.apply(this, args);
    }, timeout);
  };
}

const Utils = {
  debounce: debounce,
};

export default Utils;

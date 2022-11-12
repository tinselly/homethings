function debounce(func, timeout = 250) {
  let timer;
  return (...args) => {
    clearTimeout(timer);
    timer = setTimeout(() => {
      func.apply(this, args);
    }, timeout);
  };
}

const RAND_ICONS = [
  "👽",
  "🛸",
  "💡",
  "😁",
  "🤖",
  "👾",
  "👁️",
  "🧠",
  "🎉",
  "🧸",
  "🪩",
  "🧬",
  "🧪",
  "🥕",
  "🚀",
  "❤️",
  "☢️",
];

const Utils = {
  debounce: debounce,
  randomIcon: () => RAND_ICONS[Math.floor(Math.random() * RAND_ICONS.length)],
};

export default Utils;

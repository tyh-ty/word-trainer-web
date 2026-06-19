const fallbackWords = [
  { id: "fallback-1", spelling: "algorithm", meaning: "n. 算法", example: "A good algorithm can solve the problem faster.", level: "自定义" },
  { id: "fallback-2", spelling: "array", meaning: "n. 数组", example: "An array stores many values of the same type.", level: "自定义" },
  { id: "fallback-3", spelling: "binary", meaning: "adj. 二进制的", example: "Computers use binary numbers.", level: "自定义" },
  { id: "fallback-4", spelling: "compile", meaning: "v. 编译", example: "We need to compile the program first.", level: "自定义" }
];

const baseWords = Array.isArray(window.CET_WORDS) && window.CET_WORDS.length > 0
  ? window.CET_WORDS
  : fallbackWords;

const vocabMeta = window.CET_VOCAB_META || {
  source: "local fallback",
  cet4Count: 0,
  cet6Count: 0,
  totalCount: baseWords.length
};

const storagePrefix = "word-trainer-web-v3";
const maxWordListItems = 300;
const petBaseUrl = "http://127.0.0.1:18080";
const webTalkCooldownMs = 80 * 1000;
const wrongReviewDelayMs = 60 * 1000;
const fourHoursMs = 4 * 60 * 60 * 1000;
const twoDaysMs = 2 * 24 * 60 * 60 * 1000;
const evolveMilestones = [50, 150, 300, 500];
const defaultWebAiSettings = {
  enabled: false,
  endpoint: "https://api.deepseek.com/v1/chat/completions",
  model: "deepseek-chat",
  apiKey: ""
};
const idlePetActions = ["nod", "wave", "cheer", "hop"];
const petClickPhrases = [
  "我在这里，继续背词吧。",
  "点到我啦，下一题也稳住。",
  "查到不会读的词，我可以帮你朗读。",
  "今天多背几个，我会记住进度。"
];
const petIdlePhrases = [
  "先把到期词清掉，记忆会更稳。",
  "查过的词别放着，练一轮才算真的收下。",
  "不会读就点朗读，我可以马上念。",
  "错题不多时最适合立刻补一遍。",
  "今天的小目标还在等你。"
];
const reviewDelayByStreakMs = [
  5 * 60 * 1000,
  24 * 60 * 60 * 1000,
  3 * 24 * 60 * 60 * 1000,
  7 * 24 * 60 * 60 * 1000,
  14 * 24 * 60 * 60 * 1000
];

const state = {
  username: "",
  page: "practice",
  mode: "choice",
  level: "all",
  reviewOnly: false,
  dueOnly: false,
  lookupOnly: false,
  customWords: [],
  progress: {},
  lookupPlan: {},
  history: [],
  petEvents: {},
  dailyGoal: 30,
  petConnected: false,
  webAiSettings: { ...defaultWebAiSettings },
  lastWebTalkAt: 0,
  webTalkBusy: false,
  currentQuestion: null
};

const elements = {
  authScreen: document.querySelector("#authScreen"),
  appShell: document.querySelector("#appShell"),
  authForm: document.querySelector("#authForm"),
  loginUsername: document.querySelector("#loginUsername"),
  loginPassword: document.querySelector("#loginPassword"),
  authMessage: document.querySelector("#authMessage"),
  authWordCount: document.querySelector("#authWordCount"),
  currentUser: document.querySelector("#currentUser"),
  logoutBtn: document.querySelector("#logoutBtn"),
  petAvatar: document.querySelector("#petAvatar"),
  petSprite: document.querySelector("#petSprite"),
  petShadow: document.querySelector("#petShadow"),
  petMoodText: document.querySelector("#petMoodText"),
  petBubbleText: document.querySelector("#petBubbleText"),
  floatingPet: document.querySelector("#floatingPet"),
  floatingPetStage: document.querySelector("#floatingPetStage"),
  floatingPetBubble: document.querySelector("#floatingPetBubble"),
  floatingPetSprite: document.querySelector("#floatingPetSprite"),
  floatingPetShadow: document.querySelector("#floatingPetShadow"),
  petReminderBtn: document.querySelector("#petReminderBtn"),
  petStyleBtn: document.querySelector("#petStyleBtn"),
  petAiSettingsBtn: document.querySelector("#petAiSettingsBtn"),
  aiSettingsModal: document.querySelector("#aiSettingsModal"),
  aiSettingsForm: document.querySelector("#aiSettingsForm"),
  aiSettingsCloseBtn: document.querySelector("#aiSettingsCloseBtn"),
  aiEnabled: document.querySelector("#aiEnabled"),
  aiEndpoint: document.querySelector("#aiEndpoint"),
  aiModel: document.querySelector("#aiModel"),
  aiApiKey: document.querySelector("#aiApiKey"),
  aiSettingsHint: document.querySelector("#aiSettingsHint"),
  aiTestBtn: document.querySelector("#aiTestBtn"),
  dailyGoalInput: document.querySelector("#dailyGoalInput"),
  goalText: document.querySelector("#goalText"),
  goalProgress: document.querySelector("#goalProgress"),
  navLinks: document.querySelectorAll(".nav-link"),
  pagePanels: document.querySelectorAll(".app-page"),
  pageEyebrow: document.querySelector("#pageEyebrow"),
  pageTitle: document.querySelector("#pageTitle"),
  pageDescription: document.querySelector("#pageDescription"),
  levelBadge: document.querySelector("#levelBadge"),
  levelButtons: document.querySelectorAll(".level-btn"),
  totalAnswered: document.querySelector("#totalAnswered"),
  accuracy: document.querySelector("#accuracy"),
  wrongCount: document.querySelector("#wrongCount"),
  wordCount: document.querySelector("#wordCount"),
  wrongList: document.querySelector("#wrongList"),
  reviewWrongBtn: document.querySelector("#reviewWrongBtn"),
  reviewDueBtn: document.querySelector("#reviewDueBtn"),
  reviewLookupBtn: document.querySelector("#reviewLookupBtn"),
  practiceLookupBtn: document.querySelector("#practiceLookupBtn"),
  planHeadline: document.querySelector("#planHeadline"),
  planDetail: document.querySelector("#planDetail"),
  planDueCount: document.querySelector("#planDueCount"),
  planLookupCount: document.querySelector("#planLookupCount"),
  planWrongCount: document.querySelector("#planWrongCount"),
  planNewCount: document.querySelector("#planNewCount"),
  planDueBtn: document.querySelector("#planDueBtn"),
  planLookupBtn: document.querySelector("#planLookupBtn"),
  planWrongBtn: document.querySelector("#planWrongBtn"),
  todayLabel: document.querySelector("#todayLabel"),
  todayAnswered: document.querySelector("#todayAnswered"),
  todayAccuracy: document.querySelector("#todayAccuracy"),
  dueReviewCount: document.querySelector("#dueReviewCount"),
  markedReviewCount: document.querySelector("#markedReviewCount"),
  lookupCount: document.querySelector("#lookupCount"),
  lookupPlanCount: document.querySelector("#lookupPlanCount"),
  modeButtons: document.querySelectorAll(".mode-btn"),
  nextBtn: document.querySelector("#nextBtn"),
  speakBtn: document.querySelector("#speakBtn"),
  markReviewBtn: document.querySelector("#markReviewBtn"),
  questionType: document.querySelector("#questionType"),
  questionText: document.querySelector("#questionText"),
  questionHint: document.querySelector("#questionHint"),
  questionMeta: document.querySelector("#questionMeta"),
  answerArea: document.querySelector("#answerArea"),
  resultLine: document.querySelector("#resultLine"),
  searchForm: document.querySelector("#searchForm"),
  searchInput: document.querySelector("#searchInput"),
  searchResult: document.querySelector("#searchResult"),
  addWordForm: document.querySelector("#addWordForm"),
  newWord: document.querySelector("#newWord"),
  newMeaning: document.querySelector("#newMeaning"),
  newExample: document.querySelector("#newExample"),
  saveState: document.querySelector("#saveState"),
  wordList: document.querySelector("#wordList"),
  wordListInfo: document.querySelector("#wordListInfo"),
  clearCustomBtn: document.querySelector("#clearCustomBtn")
};

const levelLabels = {
  all: "全部",
  CET4: "四级",
  CET6: "六级",
  custom: "自定义"
};

const pageMeta = {
  practice: {
    eyebrow: "Practice",
    title: "练习",
    description: "选择题、拼写题和混合练习都在这个页面完成。"
  },
  search: {
    eyebrow: "Lookup",
    title: "查词",
    description: "输入英文或中文释义，快速检索完整词库和自定义单词。"
  },
  add: {
    eyebrow: "Custom Words",
    title: "添加单词",
    description: "把课程、作业或易错词加入自己的自定义词库。"
  },
  words: {
    eyebrow: "Word Bank",
    title: "词库",
    description: "浏览当前筛选范围内的词条，清空或管理自定义内容。"
  },
  stats: {
    eyebrow: "Progress",
    title: "统计与错题",
    description: "查看答题数量、正确率和错题排行，也可以进入错题专项练习。"
  }
};

const questionTypes = {
  choice: {
    label: "选择题",
    create(word) {
      const pool = activeWords().filter((item) => wordKey(item) !== wordKey(word));
      const choices = shuffle([word, ...shuffle(pool).slice(0, 3)]);
      return {
        type: "choice",
        word,
        prompt: word.spelling,
        hint: "选择这个单词的中文意思",
        choices
      };
    },
    render(question) {
      const grid = document.createElement("div");
      grid.className = "choice-grid";

      question.choices.forEach((choice) => {
        const button = document.createElement("button");
        button.className = "choice-button";
        button.type = "button";
        button.textContent = choice.meaning;
        button.addEventListener("click", () => {
          const ok = wordKey(choice) === wordKey(question.word);
          button.classList.add(ok ? "correct" : "wrong");
          submitAnswer(ok, question.word);
          lockChoices(grid, question.word.meaning);
        });
        grid.appendChild(button);
      });

      elements.answerArea.appendChild(grid);
    }
  },
  spelling: {
    label: "拼写题",
    create(word) {
      return {
        type: "spelling",
        word,
        prompt: word.meaning,
        hint: "根据中文意思写出英文单词"
      };
    },
    render(question) {
      const form = document.createElement("form");
      form.className = "spelling-row";

      const input = document.createElement("input");
      input.placeholder = "输入英文拼写";
      input.autocomplete = "off";

      const button = document.createElement("button");
      button.className = "secondary-button";
      button.type = "submit";
      button.textContent = "提交";

      form.append(input, button);
      form.addEventListener("submit", (event) => {
        event.preventDefault();
        const ok = normalize(input.value) === wordKey(question.word);
        input.disabled = true;
        button.disabled = true;
        submitAnswer(ok, question.word);
      });

      elements.answerArea.appendChild(form);
      input.focus();
    }
  }
};

function storageKey(name) {
  return `${storagePrefix}:${name}`;
}

function readJson(key, fallback) {
  try {
    const raw = localStorage.getItem(key);
    return raw ? JSON.parse(raw) : fallback;
  } catch {
    return fallback;
  }
}

function writeJson(key, value) {
  localStorage.setItem(key, JSON.stringify(value));
}

function normalize(value) {
  return String(value || "").trim().toLowerCase();
}

function cleanText(value) {
  return String(value || "").replace(/\s+/g, " ").trim();
}

function cleanAiBubbleText(value) {
  let text = cleanText(value).replace(/^["'“”‘’]+|["'“”‘’]+$/g, "");
  if (text.length > 80) {
    text = `${text.slice(0, 80)}...`;
  }
  return text;
}

function normalizeWebAiSettings(settings = {}) {
  return {
    enabled: Boolean(settings.enabled),
    endpoint: cleanText(settings.endpoint) || defaultWebAiSettings.endpoint,
    model: cleanText(settings.model) || defaultWebAiSettings.model,
    apiKey: cleanText(settings.apiKey)
  };
}

function loadWebAiSettings() {
  state.webAiSettings = normalizeWebAiSettings(readJson(storageKey("web-ai-settings"), defaultWebAiSettings));
}

function saveWebAiSettings(settings) {
  state.webAiSettings = normalizeWebAiSettings(settings);
  writeJson(storageKey("web-ai-settings"), state.webAiSettings);
}

function fillWebAiSettingsForm() {
  const settings = state.webAiSettings;
  elements.aiEnabled.checked = settings.enabled;
  elements.aiEndpoint.value = settings.endpoint;
  elements.aiModel.value = settings.model;
  elements.aiApiKey.value = settings.apiKey;
}

function readWebAiSettingsForm() {
  return normalizeWebAiSettings({
    enabled: elements.aiEnabled.checked,
    endpoint: elements.aiEndpoint.value,
    model: elements.aiModel.value,
    apiKey: elements.aiApiKey.value
  });
}

function showAiSettingsHint(message, ok = false) {
  elements.aiSettingsHint.textContent = message;
  elements.aiSettingsHint.className = `settings-hint ${ok ? "ok" : "bad"}`;
}

function openAiSettings() {
  fillWebAiSettingsForm();
  showAiSettingsHint("开启后，网页宠会根据当前页面、题目和学习计划随机说话。", true);
  elements.aiSettingsModal.classList.remove("hidden");
  elements.aiEndpoint.focus();
}

function closeAiSettings() {
  elements.aiSettingsModal.classList.add("hidden");
}

function todayKey(time = Date.now()) {
  const date = new Date(time);
  const year = date.getFullYear();
  const month = String(date.getMonth() + 1).padStart(2, "0");
  const day = String(date.getDate()).padStart(2, "0");
  return `${year}-${month}-${day}`;
}

function wordKey(word) {
  return normalize(word.spelling);
}

function randomItem(items) {
  return items[Math.floor(Math.random() * items.length)];
}

function shuffle(items) {
  return [...items].sort(() => Math.random() - 0.5);
}

function ensureStat(word) {
  const key = wordKey(word);
  const stat = state.progress[key] || {};
  stat.correct = Number(stat.correct || 0);
  stat.wrong = Number(stat.wrong || 0);
  stat.streak = Number(stat.streak || 0);
  stat.lastSeen = Number(stat.lastSeen || 0);
  stat.dueAt = Number(stat.dueAt || 0);
  stat.marked = Boolean(stat.marked);
  stat.meaning = word.meaning || stat.meaning || "";
  state.progress[key] = stat;
  return stat;
}

function nextReviewDelay(ok, streak) {
  if (!ok) {
    return wrongReviewDelayMs;
  }
  const index = Math.min(Math.max(streak, 0), reviewDelayByStreakMs.length - 1);
  return reviewDelayByStreakMs[index];
}

function isDueStat(stat, now = Date.now()) {
  return Boolean(stat?.marked) || (Number(stat?.dueAt || 0) > 0 && Number(stat.dueAt) <= now);
}

function dueWords() {
  const now = Date.now();
  const lookupKeys = new Set(Object.keys(state.lookupPlan || {}));
  return activeWords().filter((word) => {
    const key = wordKey(word);
    return !lookupKeys.has(key) && isDueStat(state.progress[key], now);
  });
}

function wrongPracticeWords() {
  return activeWords().filter((word) => Number(state.progress[wordKey(word)]?.wrong || 0) > 0);
}

function lookupPlanWords() {
  const planned = new Set(Object.keys(state.lookupPlan || {}));
  return activeWords()
    .filter((word) => planned.has(wordKey(word)))
    .sort((left, right) => {
      const a = state.lookupPlan[wordKey(left)] || {};
      const b = state.lookupPlan[wordKey(right)] || {};
      return Number(b.priority || 0) - Number(a.priority || 0)
        || Number(b.lastLookedUp || 0) - Number(a.lastLookedUp || 0);
    });
}

function getStudyPlan() {
  const dueCount = dueWords().length;
  const lookupCount = lookupPlanWords().length;
  const wrongCount = wrongPracticeWords().length;
  const todayCount = todayEvents().length;
  const remainingGoal = Math.max(0, Number(state.dailyGoal || 0) - todayCount);

  if (state.dueOnly) {
    return {
      headline: dueCount > 0 ? `正在练 ${dueCount} 个到期词` : "到期复习已清空",
      detail: dueCount > 0 ? "当前题目只从到期复习词里抽取。" : "可以回到普通练习，或查几个新词加入计划。",
      petMessage: dueCount > 0 ? `先把 ${dueCount} 个到期词收掉。` : "到期词已经练完啦。",
      dueCount,
      lookupCount,
      wrongCount,
      remainingGoal,
      mode: "due"
    };
  }

  if (state.lookupOnly) {
    return {
      headline: lookupCount > 0 ? `正在练 ${lookupCount} 个查词计划词` : "查词计划已清空",
      detail: lookupCount > 0 ? "答对后会从查词计划移出，答错会继续保留。" : "去查词页查询新词后，这里会重新出现待练内容。",
      petMessage: lookupCount > 0 ? `查词计划还剩 ${lookupCount} 个词。` : "查词计划清空啦。",
      dueCount,
      lookupCount,
      wrongCount,
      remainingGoal,
      mode: "lookup"
    };
  }

  if (state.reviewOnly) {
    return {
      headline: wrongCount > 0 ? `正在补 ${wrongCount} 个错题词` : "错题专项已清空",
      detail: wrongCount > 0 ? "当前题目只从错题词里抽取。" : "继续普通练习，新的错题会自动回到这里。",
      petMessage: wrongCount > 0 ? `错题还剩 ${wrongCount} 个。` : "错题专项暂时清空啦。",
      dueCount,
      lookupCount,
      wrongCount,
      remainingGoal,
      mode: "wrong"
    };
  }

  if (dueCount > 0) {
    return {
      headline: `先复习 ${dueCount} 个到期词`,
      detail: `这些词已经到复习时间，先做它们，再继续查词计划和新题。`,
      petMessage: `现在有 ${dueCount} 个词该复习啦。`,
      dueCount,
      lookupCount,
      wrongCount,
      remainingGoal,
      mode: "due"
    };
  }

  if (lookupCount > 0) {
    return {
      headline: `练 ${lookupCount} 个查过的词`,
      detail: `你查过的词会自动进计划，建议马上练一轮，避免只查不记。`,
      petMessage: `查词计划里有 ${lookupCount} 个词，练一下更牢。`,
      dueCount,
      lookupCount,
      wrongCount,
      remainingGoal,
      mode: "lookup"
    };
  }

  if (wrongCount > 0) {
    return {
      headline: `补 ${wrongCount} 个错题词`,
      detail: `错题会保留在专项练习里，先把易错词稳定下来。`,
      petMessage: `还有 ${wrongCount} 个错题词可以补。`,
      dueCount,
      lookupCount,
      wrongCount,
      remainingGoal,
      mode: "wrong"
    };
  }

  if (remainingGoal > 0) {
    return {
      headline: `今日目标还差 ${remainingGoal} 题`,
      detail: `按当前词库继续练习，答题记录会自动安排后续复习时间。`,
      petMessage: `今天还差 ${remainingGoal} 题，继续冲一下。`,
      dueCount,
      lookupCount,
      wrongCount,
      remainingGoal,
      mode: "new"
    };
  }

  return {
    headline: "今天的计划完成了",
    detail: "可以查几个新词加入计划，或者切换词库继续练习。",
    petMessage: "今天的计划完成啦，可以查几个新词。",
    dueCount,
    lookupCount,
    wrongCount,
    remainingGoal,
    mode: "done"
  };
}

function renderStudyPlan(plan = getStudyPlan()) {
  elements.planHeadline.textContent = plan.headline;
  elements.planDetail.textContent = plan.detail;
  elements.planDueCount.textContent = plan.dueCount;
  elements.planLookupCount.textContent = plan.lookupCount;
  elements.planWrongCount.textContent = plan.wrongCount;
  elements.planNewCount.textContent = plan.remainingGoal;
  elements.planDueBtn.disabled = plan.dueCount === 0;
  elements.planLookupBtn.disabled = plan.lookupCount === 0;
  elements.planWrongBtn.disabled = plan.wrongCount === 0;
}

function todayEvents() {
  const today = todayKey();
  return state.history.filter((item) => item.day === today);
}

function saveHistory() {
  state.history = state.history.slice(-1200);
  writeJson(storageKey(`history:${state.username}`), state.history);
}

function saveDailyGoal() {
  writeJson(storageKey(`goal:${state.username}`), state.dailyGoal);
}

function savePetEvents() {
  writeJson(storageKey(`pet-events:${state.username}`), state.petEvents);
}

function saveLookupPlan() {
  writeJson(storageKey(`lookup-plan:${state.username}`), state.lookupPlan);
}

function setPetMood(mood, message) {
  const labels = {
    sleep: "沉睡中",
    hatch: "破壳啦",
    hungry: "有点饿",
    happy: "开心",
    evolve: "进化中",
    leave: "离家警告",
    study: "陪你背词",
    lookup: "查词规划中",
    read: "朗读中"
  };
  elements.petMoodText.textContent = labels[mood] || "陪你背词";
  elements.petBubbleText.textContent = message || "继续背单词吧。";
  playPetAction(mood || "study", actionForMood(mood));
  elements.floatingPetBubble.textContent = message || "继续背单词吧。";
  elements.floatingPetBubble.classList.add("active");
  setPetMood.activeUntil = Date.now() + 5000;
  window.clearTimeout(setPetMood.timer);
  setPetMood.timer = window.setTimeout(() => {
    elements.petMoodText.textContent = "陪你背词";
    elements.petBubbleText.textContent = "查词、复习和答题我都会帮你记。";
    playPetAction("study", "idle");
    elements.floatingPetBubble.classList.remove("active");
  }, 5000);
}

function playPetAction(mood, action) {
  [elements.petAvatar, elements.floatingPet].forEach((node) => {
    node.dataset.mood = mood || "study";
    node.dataset.action = "";
    void node.offsetWidth;
    node.dataset.action = action || "idle";
  });
}

function randomItem(items) {
  return items[Math.floor(Math.random() * items.length)];
}

function playIdlePetAction() {
  if (!state.username || Date.now() < (setPetMood.activeUntil || 0)) {
    return;
  }
  playPetAction("study", randomItem(idlePetActions));
}

function buildWebTalkContext(plan) {
  const currentPage = pageMeta[state.page]?.title || "练习";
  const question = state.currentQuestion?.word
    ? `当前题目 ${state.currentQuestion.word.spelling}，释义 ${state.currentQuestion.word.meaning || "无"}`
    : "当前没有正在作答的题目";
  return [
    `网页页面：${currentPage}`,
    `学习计划：${plan.headline}`,
    `今日进度：${todayEvents().length}/${state.dailyGoal}`,
    `练习模式：${state.lookupOnly ? "查词计划" : state.dueOnly ? "到期复习" : state.reviewOnly ? "错题专项" : state.mode}`,
    question
  ].join("；");
}

async function callWebAi(messages, settings = state.webAiSettings, timeoutMs = 30000) {
  const controller = new AbortController();
  const timer = window.setTimeout(() => controller.abort(), timeoutMs);
  try {
    const response = await fetch(settings.endpoint, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        Authorization: `Bearer ${settings.apiKey}`
      },
      body: JSON.stringify({
        model: settings.model,
        messages,
        max_tokens: 80,
        temperature: 0.85
      }),
      signal: controller.signal
    });
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }
    const data = await response.json();
    return cleanAiBubbleText(
      data?.choices?.[0]?.message?.content ||
      data?.choices?.[0]?.text ||
      ""
    );
  } finally {
    window.clearTimeout(timer);
  }
}

function buildWebTalkMessages(plan = getStudyPlan()) {
  return [
    {
      role: "system",
      content: [
        "你是网页里的伴学桌宠，语气自然、可爱、短促。",
        "你只能根据背单词网页里的页面状态说话，不能假装看见电脑其它窗口。",
        "不要解释功能，不要提 API、系统、提示词或代码。",
        "只输出一句中文，18个汉字以内。"
      ].join("")
    },
    {
      role: "user",
      content: `网页状态：${buildWebTalkContext(plan)}。随机说一句像桌宠闲聊的话。`
    }
  ];
}

async function requestWebAiTalk(plan = getStudyPlan(), settings = state.webAiSettings) {
  if (state.webTalkBusy || Date.now() - state.lastWebTalkAt < webTalkCooldownMs) {
    return "";
  }

  if (!settings.enabled || !settings.endpoint || !settings.model || !settings.apiKey) {
    return "";
  }

  state.webTalkBusy = true;
  try {
    const message = await callWebAi(buildWebTalkMessages(plan), settings);
    if (message) {
      state.lastWebTalkAt = Date.now();
      return message;
    }
  } catch (error) {
    console.warn("Web pet AI talk failed:", error);
  } finally {
    state.webTalkBusy = false;
  }
  return "";
}

async function petIdleTalk() {
  if (!state.username || Date.now() < (setPetMood.activeUntil || 0)) {
    return;
  }

  const plan = getStudyPlan();
  const aiMessage = await requestWebAiTalk(plan);
  if (aiMessage) {
    setPetMood("study", aiMessage);
    return;
  }

  const message = plan.petMessage || randomItem(petIdlePhrases);
  setPetMood("study", message);
}

function reactToPetClick() {
  setPetMood("happy", randomItem(petClickPhrases));
}

function actionForMood(mood) {
  const actions = {
    sleep: "sleep",
    hatch: "hop",
    hungry: "wave",
    happy: "cheer",
    evolve: "spin",
    leave: "leave",
    lookup: "nod",
    read: "nod",
    study: "idle"
  };
  return actions[mood] || "idle";
}

function applyPetStyle(style) {
  const nextStyle = style === "nailong" ? "nailong" : "aimee";
  elements.petAvatar.dataset.style = nextStyle;
  elements.floatingPet.dataset.style = nextStyle;
  elements.petSprite.src = `assets/${nextStyle}_pet_hd.png`;
  elements.petShadow.src = `assets/${nextStyle}_pet_shadow.png`;
  elements.floatingPetSprite.src = `assets/${nextStyle}_pet_hd.png`;
  elements.floatingPetShadow.src = `assets/${nextStyle}_pet_shadow.png`;
  state.petEvents.petStyle = nextStyle;
  savePetEvents();
}

function togglePetStyle() {
  const current = state.petEvents.petStyle === "nailong" ? "nailong" : "aimee";
  const next = current === "aimee" ? "nailong" : "aimee";
  applyPetStyle(next);
  setPetMood("happy", next === "aimee" ? "爱弥斯上线，继续陪你背。" : "奶龙上线，冲单词！");
}

function loadFloatingPetPosition() {
  const position = readJson(storageKey(`pet-position:${state.username}`), null);
  if (!position) {
    elements.floatingPet.style.right = "24px";
    elements.floatingPet.style.bottom = "24px";
    elements.floatingPet.style.left = "";
    elements.floatingPet.style.top = "";
    return;
  }
  elements.floatingPet.style.left = `${position.x}px`;
  elements.floatingPet.style.top = `${position.y}px`;
  elements.floatingPet.style.right = "";
  elements.floatingPet.style.bottom = "";
}

function saveFloatingPetPosition(x, y) {
  writeJson(storageKey(`pet-position:${state.username}`), { x, y });
}

function bindFloatingPetDrag() {
  let dragging = false;
  let moved = false;
  let startX = 0;
  let startY = 0;
  let startLeft = 0;
  let startTop = 0;

  elements.floatingPet.addEventListener("pointerdown", (event) => {
    if (!state.username) return;
    dragging = true;
    moved = false;
    startX = event.clientX;
    startY = event.clientY;
    const rect = elements.floatingPet.getBoundingClientRect();
    startLeft = rect.left;
    startTop = rect.top;
    elements.floatingPet.setPointerCapture(event.pointerId);
  });

  elements.floatingPet.addEventListener("pointermove", (event) => {
    if (!dragging) return;
    const dx = event.clientX - startX;
    const dy = event.clientY - startY;
    if (Math.abs(dx) + Math.abs(dy) > 5) {
      moved = true;
    }
    const rect = elements.floatingPet.getBoundingClientRect();
    const maxX = Math.max(0, window.innerWidth - rect.width);
    const maxY = Math.max(0, window.innerHeight - rect.height);
    const x = Math.min(maxX, Math.max(0, startLeft + dx));
    const y = Math.min(maxY, Math.max(0, startTop + dy));
    elements.floatingPet.style.left = `${x}px`;
    elements.floatingPet.style.top = `${y}px`;
    elements.floatingPet.style.right = "";
    elements.floatingPet.style.bottom = "";
  });

  elements.floatingPet.addEventListener("pointerup", (event) => {
    if (!dragging) return;
    dragging = false;
    elements.floatingPet.releasePointerCapture(event.pointerId);
    const rect = elements.floatingPet.getBoundingClientRect();
    saveFloatingPetPosition(Math.round(rect.left), Math.round(rect.top));
    if (!moved) {
      reactToPetClick();
    }
  });

  elements.floatingPet.addEventListener("keydown", (event) => {
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      reactToPetClick();
    }
  });
}

function masteredWordCount() {
  return Object.values(state.progress).filter((stat) => Number(stat.correct || 0) > 0).length;
}

function latestStudyTime() {
  if (!state.history.length) {
    return 0;
  }
  return Math.max(...state.history.map((item) => Number(item.at || 0)));
}

function sendPetState(event, message, onceKey = "") {
  if (onceKey && state.petEvents[onceKey]) {
    return;
  }

  setPetMood(event, message);
  if (onceKey) {
    state.petEvents[onceKey] = Date.now();
    savePetEvents();
  }

  if (!state.petConnected) {
    return;
  }
  callPet("/study-state", { event, message }, 1600).catch(() => {
    state.petConnected = false;
    updatePetStatus("未连接");
  });
}

function checkLoginPetBehaviors() {
  const learned = masteredWordCount();
  const lastStudy = latestStudyTime();
  const lastOpen = Number(state.petEvents.lastOpenAt || 0);
  const now = Date.now();

  if (learned === 0) {
    sendPetState("sleep", "词汇量还是0，我先缩成蛋睡会儿", "first-sleep");
  }

  if (lastStudy > 0 && now - lastStudy >= fourHoursMs) {
    sendPetState("hungry", "连续4小时没学习，我有点饿啦", `hungry-${todayKey(now)}`);
  }

  if (lastOpen > 0 && now - lastOpen >= twoDaysMs) {
    sendPetState("leave", "连续2天没打开，我要离家出走提醒你啦", `leave-${todayKey(now)}`);
  }

  state.petEvents.lastOpenAt = now;
  savePetEvents();
}

function checkAnswerPetBehaviors(word, ok, stat) {
  if (ok && Number(stat.correct || 0) === 1 && masteredWordCount() === 1) {
    sendPetState("hatch", `正确记忆第1个单词：${word.spelling}`, "first-word");
  }

  const correctStreak = Number(state.petEvents.correctStreak || 0);
  if (ok && correctStreak > 0 && correctStreak % 5 === 0) {
    sendPetState("happy", `连续答对${correctStreak}题，太稳了`, `happy-${correctStreak}-${Date.now()}`);
  }

  const learned = masteredWordCount();
  evolveMilestones.forEach((milestone) => {
    if (learned >= milestone) {
      sendPetState("evolve", `累计词汇达到${milestone}，桌宠进化！`, `evolve-${milestone}`);
    }
  });
}

function updatePetStatus(text) {
  if (state.petConnected && text) {
    setPetMood("study", text);
  }
}

async function callPet(path, payload, timeoutMs = 1600) {
  const controller = new AbortController();
  const timer = window.setTimeout(() => controller.abort(), timeoutMs);
  try {
    const options = payload
      ? {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
          signal: controller.signal
        }
      : { method: "GET", signal: controller.signal };
    const response = await fetch(`${petBaseUrl}${path}`, options);
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }
    return await response.json();
  } finally {
    window.clearTimeout(timer);
  }
}

async function refreshPetConnection(showMessage = false) {
  try {
    const data = await callPet("/status", null, 1200);
    state.petConnected = Boolean(data.ok);
    if (showMessage && state.petConnected) {
      callPet("/bubble", { message: "网页已连接，开始背单词吧" }).catch(() => {});
    }
    if (state.username && state.petConnected) {
      checkLoginPetBehaviors();
    }
    return state.petConnected;
  } catch {
    state.petConnected = false;
    return false;
  }
}

function browserSpeak(text) {
  if (!("speechSynthesis" in window)) {
    showResult("当前浏览器不支持朗读，请先启动桌宠。", false);
    return;
  }
  window.speechSynthesis.cancel();
  const utterance = new SpeechSynthesisUtterance(text);
  utterance.lang = "en-US";
  utterance.rate = 0.88;
  window.speechSynthesis.speak(utterance);
}

async function speakWord(word = state.currentQuestion?.word) {
  if (!word) {
    return;
  }

  setPetMood("read", `我来读：${word.spelling}`);
  if (state.petConnected) {
    try {
      const data = await callPet("/speak", {
        word: word.spelling,
        meaning: word.meaning || ""
      }, 700);
      if (data.ok) {
        return;
      }
    } catch {
      state.petConnected = false;
    }
  }

  browserSpeak(word.spelling);
  refreshPetConnection(false).then((connected) => {
    if (!connected) return;
    callPet("/bubble", {
      message: word.meaning ? `我来读：${word.spelling}\n${word.meaning}` : `我来读：${word.spelling}`
    }, 900).catch(() => {
      state.petConnected = false;
    });
  });
}

function syncPetRecord(word, ok, stat) {
  if (!state.petConnected) {
    return;
  }
  callPet("/record", {
    word: word.spelling,
    meaning: word.meaning || "",
    ok,
    correct: stat.correct,
    wrong: stat.wrong,
    streak: stat.streak,
    lastSeen: stat.lastSeen,
    dueAt: stat.dueAt,
    marked: stat.marked
  }, 1600).catch(() => {
    state.petConnected = false;
  });
}

function markCurrentForReview() {
  const word = state.currentQuestion?.word;
  if (!word) {
    return;
  }
  const key = wordKey(word);
  const stat = ensureStat(word);
  stat.marked = true;
  stat.dueAt = Date.now();
  stat.meaning = word.meaning || "";
  if (state.lookupPlan[key]) {
    delete state.lookupPlan[key];
    saveLookupPlan();
  }
  saveProgress();
  syncPetRecord(word, false, stat);
  showResult(`已加入复习：${word.spelling}`, true);
  renderStats();
}

function addToLookupPlan(word, source = "lookup") {
  if (!word) {
    return;
  }
  const key = wordKey(word);
  const now = Date.now();
  const plan = state.lookupPlan[key] || {
    word: word.spelling,
    meaning: word.meaning || "",
    count: 0,
    priority: 0,
    firstLookedUp: now
  };
  plan.word = word.spelling;
  plan.meaning = word.meaning || plan.meaning || "";
  plan.count = Number(plan.count || 0) + 1;
  plan.lastLookedUp = now;
  plan.priority = Number(plan.priority || 0) + (source === "manual" ? 3 : 1);
  state.lookupPlan[key] = plan;

  const stat = ensureStat(word);
  stat.meaning = word.meaning || "";

  saveLookupPlan();
  saveProgress();
  renderStats();
}

function updateLookupPlanAfterAnswer(word, ok, answeredAt) {
  const key = wordKey(word);
  const plan = state.lookupPlan[key];
  if (!plan) {
    return;
  }

  if (ok) {
    delete state.lookupPlan[key];
  } else {
    plan.lastPracticed = answeredAt;
    plan.lastWrong = answeredAt;
    plan.priority = Number(plan.priority || 0) + 2;
    state.lookupPlan[key] = plan;
  }
  saveLookupPlan();
}

async function askPetToRemind() {
  const connected = state.petConnected || await refreshPetConnection(false);
  if (!connected) {
    state.dueOnly = true;
    state.lookupOnly = false;
    state.reviewOnly = false;
    setPetMood("study", "先练到期复习词。");
    navigateToPage("practice");
    nextQuestion();
    return;
  }

  const due = dueWords();
  const message = due.length > 0
    ? `现在有 ${due.length} 个词该复习了`
    : "暂时没有到期词，先练新题吧";
  callPet("/bubble", { message }).catch(() => {});
  setPetMood("study", message);
  state.dueOnly = due.length > 0;
  state.lookupOnly = false;
  state.reviewOnly = false;
  navigateToPage("practice");
  nextQuestion();
}

function loadAccounts() {
  const accounts = readJson(storageKey("accounts"), {});
  if (!accounts.student) {
    accounts.student = {
      password: "123456",
      createdAt: "2026-06-13"
    };
    writeJson(storageKey("accounts"), accounts);
  }
  return accounts;
}

function saveAccounts(accounts) {
  writeJson(storageKey("accounts"), accounts);
}

function showAuthMessage(message, ok = false) {
  elements.authMessage.textContent = message;
  elements.authMessage.className = `auth-message ${ok ? "ok" : "bad"}`;
}

function pageFromHash() {
  const page = window.location.hash.replace(/^#/, "");
  return pageMeta[page] ? page : "practice";
}

function showPage(page) {
  const targetPage = pageMeta[page] ? page : "practice";
  const meta = pageMeta[targetPage];
  state.page = targetPage;

  elements.navLinks.forEach((link) => {
    const active = link.dataset.page === targetPage;
    link.classList.toggle("active", active);
    if (active) {
      link.setAttribute("aria-current", "page");
    } else {
      link.removeAttribute("aria-current");
    }
  });

  elements.pagePanels.forEach((panel) => {
    panel.classList.toggle("active", panel.dataset.page === targetPage);
  });

  elements.pageEyebrow.textContent = meta.eyebrow;
  elements.pageTitle.textContent = meta.title;
  elements.pageDescription.textContent = meta.description;
}

function navigateToPage(page) {
  if (!pageMeta[page]) {
    return;
  }

  if (window.location.hash === `#${page}`) {
    showPage(page);
    return;
  }
  window.location.hash = page;
}

function loginUser(username) {
  state.username = username;
  localStorage.setItem(storageKey("last-user"), username);
  loadUserData();
  elements.currentUser.textContent = username;
  elements.authScreen.classList.add("hidden");
  elements.appShell.classList.remove("hidden");
  elements.floatingPet.classList.remove("hidden");
  loadFloatingPetPosition();
  elements.loginPassword.value = "";
  renderAll();
  nextQuestion();
  checkLoginPetBehaviors();
  refreshPetConnection(false);
}

function loadUserData() {
  state.customWords = readJson(storageKey(`custom:${state.username}`), []);
  state.progress = readJson(storageKey(`progress:${state.username}`), {});
  state.lookupPlan = readJson(storageKey(`lookup-plan:${state.username}`), {});
  state.history = readJson(storageKey(`history:${state.username}`), []);
  state.petEvents = readJson(storageKey(`pet-events:${state.username}`), {});
  state.dailyGoal = Number(readJson(storageKey(`goal:${state.username}`), 30)) || 30;
  state.reviewOnly = false;
  state.dueOnly = false;
  state.lookupOnly = false;
  elements.dailyGoalInput.value = state.dailyGoal;
  applyPetStyle(state.petEvents.petStyle || "aimee");
}

function saveProgress() {
  writeJson(storageKey(`progress:${state.username}`), state.progress);
  flashSaveState();
}

function saveCustomWords() {
  writeJson(storageKey(`custom:${state.username}`), state.customWords);
  flashSaveState();
}

function allWords() {
  return [...baseWords, ...state.customWords];
}

function levelMatches(word) {
  if (state.level === "all") {
    return true;
  }
  if (state.level === "custom") {
    return word.level === "自定义";
  }
  return String(word.level || "").includes(state.level);
}

function activeWords() {
  return allWords().filter(levelMatches);
}

function availableWords() {
  const words = activeWords();
  if (state.lookupOnly) {
    const planned = new Set(lookupPlanWords().map(wordKey));
    return words.filter((word) => planned.has(wordKey(word)));
  }

  if (state.dueOnly) {
    const dueSpellings = new Set(dueWords().map(wordKey));
    return words.filter((word) => dueSpellings.has(wordKey(word)));
  }

  if (!state.reviewOnly) {
    return words;
  }

  const wrongSpellings = new Set(
    Object.entries(state.progress)
      .filter(([, stat]) => stat.wrong > 0)
      .map(([spelling]) => spelling)
  );
  return words.filter((word) => wrongSpellings.has(wordKey(word)));
}

function nextQuestion() {
  const source = availableWords();
  if (source.length === 0) {
    const title = state.lookupOnly ? "查词计划为空" : state.dueOnly ? "暂无到期复习" : state.reviewOnly ? "暂无错题" : "当前范围没有单词";
    const hint = state.lookupOnly
      ? "去查词页查单词，系统会自动把查过的词加入计划。"
      : state.dueOnly
      ? "可以先继续练习新单词，系统会按答题情况安排下次复习。"
      : state.reviewOnly
      ? "完成练习后，答错的单词会出现在错题专项里。"
      : "可以切换词库范围，或在自定义词库里添加新单词。";
    renderEmptyQuestion(title, hint);
    return;
  }

  const mode = state.mode === "mixed" ? randomItem(["choice", "spelling"]) : state.mode;
  const word = randomItem(source);
  state.currentQuestion = questionTypes[mode].create(word);
  renderQuestion();
}

function renderEmptyQuestion(title, hint) {
  state.currentQuestion = null;
  elements.questionType.textContent = state.lookupOnly ? "查词计划" : state.dueOnly ? "到期复习" : state.reviewOnly ? "错题专项" : "练习";
  elements.questionText.textContent = title;
  elements.questionText.classList.toggle("long-prompt", String(title).length > 18);
  elements.questionHint.textContent = hint;
  elements.questionMeta.textContent = "";
  elements.answerArea.innerHTML = "";
  elements.resultLine.textContent = "";
  elements.resultLine.className = "result-line";
  elements.speakBtn.disabled = true;
  elements.markReviewBtn.disabled = true;
}

function renderQuestion() {
  const question = state.currentQuestion;
  elements.questionType.textContent = state.dueOnly
    ? `到期复习 · ${questionTypes[question.type].label}`
    : state.lookupOnly
    ? `查词计划 · ${questionTypes[question.type].label}`
    : state.reviewOnly
    ? `错题专项 · ${questionTypes[question.type].label}`
    : questionTypes[question.type].label;
  elements.questionText.textContent = question.prompt;
  elements.questionText.classList.toggle("long-prompt", String(question.prompt).length > 18);
  elements.questionHint.textContent = question.hint;
  elements.questionMeta.textContent = question.word.level || "";
  elements.answerArea.innerHTML = "";
  elements.resultLine.textContent = "";
  elements.resultLine.className = "result-line";
  elements.speakBtn.disabled = false;
  elements.markReviewBtn.disabled = false;
  questionTypes[question.type].render(question);
}

function submitAnswer(ok, word) {
  const key = wordKey(word);
  const stat = ensureStat(word);
  const answeredAt = Date.now();
  if (ok) {
    stat.correct += 1;
    stat.streak += 1;
    stat.marked = false;
    state.petEvents.correctStreak = Number(state.petEvents.correctStreak || 0) + 1;
  } else {
    stat.wrong += 1;
    stat.streak = 0;
    state.petEvents.correctStreak = 0;
  }
  stat.lastSeen = answeredAt;
  stat.dueAt = answeredAt + nextReviewDelay(ok, stat.streak);
  stat.meaning = word.meaning || "";
  state.progress[key] = stat;
  updateLookupPlanAfterAnswer(word, ok, answeredAt);

  state.history.push({
    at: answeredAt,
    day: todayKey(answeredAt),
    word: word.spelling,
    ok,
    mode: state.currentQuestion?.type || state.mode,
    level: word.level || ""
  });

  saveProgress();
  saveHistory();
  savePetEvents();
  syncPetRecord(word, ok, stat);
  checkAnswerPetBehaviors(word, ok, stat);

  const detail = word.example ? `；${word.example}` : "";
  showResult(ok ? `正确：${word.spelling}${detail}` : `答案：${word.spelling}：${word.meaning}`, ok);
  renderStats();
}

function showResult(message, ok) {
  elements.resultLine.textContent = message;
  elements.resultLine.className = `result-line ${ok ? "ok" : "bad"}`;
}

function lockChoices(grid, answerMeaning) {
  grid.querySelectorAll("button").forEach((button) => {
    button.disabled = true;
    if (button.textContent === answerMeaning) {
      button.classList.add("correct");
    }
  });
}

function renderStats() {
  const stats = Object.values(state.progress);
  const correct = stats.reduce((sum, item) => sum + item.correct, 0);
  const wrong = stats.reduce((sum, item) => sum + item.wrong, 0);
  const total = correct + wrong;
  const wrongWords = stats.filter((item) => item.wrong > 0).length;
  const currentWords = activeWords();
  const today = todayEvents();
  const todayCorrect = today.filter((item) => item.ok).length;
  const dueCount = dueWords().length;
  const markedCount = stats.filter((item) => item.marked).length;
  const lookupPlanCount = lookupPlanWords().length;
  const goalDone = Math.min(today.length, state.dailyGoal);
  const goalPercent = state.dailyGoal > 0 ? Math.min(100, Math.round((goalDone / state.dailyGoal) * 100)) : 0;

  elements.totalAnswered.textContent = total;
  elements.accuracy.textContent = total === 0 ? "0%" : `${Math.round((correct / total) * 100)}%`;
  elements.wrongCount.textContent = wrongWords;
  elements.wordCount.textContent = currentWords.length;
  elements.levelBadge.textContent = levelLabels[state.level];
  elements.todayLabel.textContent = todayKey();
  elements.todayAnswered.textContent = today.length;
  elements.todayAccuracy.textContent = today.length === 0 ? "0%" : `${Math.round((todayCorrect / today.length) * 100)}%`;
  elements.dueReviewCount.textContent = dueCount;
  elements.markedReviewCount.textContent = markedCount;
  elements.lookupCount.textContent = lookupPlanCount;
  elements.lookupPlanCount.textContent = lookupPlanCount;
  elements.goalText.textContent = `${goalDone} / ${state.dailyGoal}`;
  elements.goalProgress.style.width = `${goalPercent}%`;
  renderStudyPlan();

  const ranked = Object.entries(state.progress)
    .filter(([, stat]) => stat.wrong > 0)
    .sort((left, right) => right[1].wrong - left[1].wrong)
    .slice(0, 6);

  elements.reviewWrongBtn.disabled = ranked.length === 0;
  elements.reviewDueBtn.disabled = dueCount === 0;
  elements.reviewLookupBtn.disabled = lookupPlanCount === 0;
  elements.practiceLookupBtn.disabled = lookupPlanCount === 0;
  elements.wrongList.innerHTML = "";
  if (ranked.length === 0) {
    elements.wrongList.appendChild(createWrongItem("暂无错题", "完成练习后这里会自动更新"));
    return;
  }

  ranked.forEach(([spelling, stat]) => {
    const word = allWords().find((item) => wordKey(item) === spelling);
    elements.wrongList.appendChild(
      createWrongItem(spelling, `${word?.meaning || ""} · 错 ${stat.wrong} 次 · 对 ${stat.correct} 次`)
    );
  });
}

function createWrongItem(title, subtitle) {
  const item = document.createElement("div");
  item.className = "wrong-item";
  const strong = document.createElement("strong");
  strong.textContent = title;
  const small = document.createElement("small");
  small.textContent = subtitle;
  item.append(strong, small);
  return item;
}

function renderWordList() {
  const source = activeWords();
  const shown = source.slice(0, maxWordListItems);
  elements.wordList.innerHTML = "";
  elements.wordListInfo.textContent =
    `当前筛选 ${source.length} 个单词，列表显示前 ${shown.length} 个；上方查词可以搜索全部词库。`;

  if (source.length === 0) {
    const empty = document.createElement("div");
    empty.className = "word-item";
    empty.textContent = "暂无单词";
    elements.wordList.appendChild(empty);
    return;
  }

  shown.forEach((word) => {
    const item = document.createElement("div");
    item.className = "word-item";
    const top = document.createElement("div");
    top.className = "word-row";
    const spelling = document.createElement("strong");
    spelling.textContent = word.spelling;
    const level = document.createElement("span");
    level.textContent = word.level || "";
    top.append(spelling, level);
    const meaning = document.createElement("small");
    meaning.textContent = word.meaning;
    const actions = document.createElement("div");
    actions.className = "word-actions";
    const speak = document.createElement("button");
    speak.className = "ghost-button";
    speak.type = "button";
    speak.textContent = "朗读";
    speak.addEventListener("click", () => speakWord(word));
    const plan = document.createElement("button");
    plan.className = "ghost-button";
    plan.type = "button";
    plan.textContent = "加入计划";
    plan.addEventListener("click", () => {
      addToLookupPlan(word, "manual");
      setPetMood("lookup", `${word.spelling} 已加入查词计划。`);
    });
    actions.append(speak, plan);
    item.append(top, meaning, actions);
    elements.wordList.appendChild(item);
  });
}

function renderSearchResults(matches, query) {
  elements.searchResult.innerHTML = "";
  if (!query) {
    elements.searchResult.textContent = "输入内容后可以查询四六级完整词库。";
    return;
  }
  if (matches.length === 0) {
    elements.searchResult.textContent = "没有找到这个单词或释义。";
    return;
  }

  matches.slice(0, 10).forEach((word) => {
    const item = document.createElement("div");
    item.className = "search-item";
    const title = document.createElement("strong");
    title.textContent = `${word.spelling} · ${word.level}`;
    const meaning = document.createElement("span");
    meaning.textContent = word.meaning;
    const actions = document.createElement("div");
    actions.className = "word-actions";
    const speak = document.createElement("button");
    speak.className = "ghost-button";
    speak.type = "button";
    speak.textContent = "朗读";
    speak.addEventListener("click", () => speakWord(word));
    const plan = document.createElement("button");
    plan.className = "ghost-button";
    plan.type = "button";
    plan.textContent = "加入计划";
    plan.addEventListener("click", () => {
      addToLookupPlan(word, "manual");
      setPetMood("lookup", `${word.spelling} 已加入查词计划。`);
    });
    actions.append(speak, plan);
    item.append(title, meaning, actions);
    if (word.example) {
      const example = document.createElement("small");
      example.textContent = word.example;
      item.appendChild(example);
    }
    elements.searchResult.appendChild(item);
  });
}

function flashSaveState(message = "已保存") {
  elements.saveState.textContent = message;
  window.clearTimeout(flashSaveState.timer);
  flashSaveState.timer = window.setTimeout(() => {
    elements.saveState.textContent = "自动保存";
  }, 1000);
}

function renderAll() {
  renderStats();
  renderWordList();
}

function startPracticeMode(mode) {
  state.reviewOnly = mode === "wrong";
  state.dueOnly = mode === "due";
  state.lookupOnly = mode === "lookup";
  renderStudyPlan();
  navigateToPage("practice");
  nextQuestion();
}

function bindEvents() {
  elements.navLinks.forEach((link) => {
    link.addEventListener("click", (event) => {
      event.preventDefault();
      navigateToPage(link.dataset.page);
    });
  });

  window.addEventListener("hashchange", () => {
    showPage(pageFromHash());
  });

  elements.authForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const action = event.submitter?.dataset.authAction || "login";
    const username = cleanText(elements.loginUsername.value) || "student";
    const password = elements.loginPassword.value;

    if (!password) {
      showAuthMessage("请输入密码。");
      return;
    }

    const accounts = loadAccounts();
    if (action === "register") {
      if (accounts[username]) {
        showAuthMessage("这个账号已经存在，可以直接登录。");
        return;
      }
      accounts[username] = {
        password,
        createdAt: new Date().toISOString()
      };
      saveAccounts(accounts);
      showAuthMessage("注册成功，正在进入主页面。", true);
      loginUser(username);
      return;
    }

    if (!accounts[username]) {
      showAuthMessage("账号不存在，可以先注册新账号。");
      return;
    }
    if (accounts[username].password !== password) {
      showAuthMessage("密码不正确。");
      return;
    }
    loginUser(username);
  });

  elements.logoutBtn.addEventListener("click", () => {
    state.username = "";
    elements.appShell.classList.add("hidden");
    elements.authScreen.classList.remove("hidden");
    elements.floatingPet.classList.add("hidden");
    elements.loginPassword.value = "";
    elements.loginUsername.focus();
  });

  elements.petReminderBtn.addEventListener("click", () => {
    askPetToRemind();
  });

  elements.petStyleBtn.addEventListener("click", () => {
    togglePetStyle();
  });

  elements.petAiSettingsBtn.addEventListener("click", () => {
    openAiSettings();
  });

  elements.aiSettingsCloseBtn.addEventListener("click", () => {
    closeAiSettings();
  });

  elements.aiSettingsModal.addEventListener("click", (event) => {
    if (event.target === elements.aiSettingsModal) {
      closeAiSettings();
    }
  });

  elements.aiSettingsForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const settings = readWebAiSettingsForm();
    saveWebAiSettings(settings);
    state.lastWebTalkAt = 0;
    showAiSettingsHint(settings.enabled ? "已保存，网页宠会自己调用远程 API 闲聊。" : "已保存，AI 闲聊已关闭。", true);
  });

  elements.aiTestBtn.addEventListener("click", async () => {
    const settings = readWebAiSettingsForm();
    if (!settings.endpoint || !settings.model || !settings.apiKey) {
      showAiSettingsHint("请先填 endpoint、model 和 API key。");
      return;
    }

    elements.aiTestBtn.disabled = true;
    showAiSettingsHint("正在测试远程 API...");
    try {
      const message = await callWebAi(buildWebTalkMessages(), settings, 30000);
      if (!message) {
        throw new Error("empty response");
      }
      saveWebAiSettings({ ...settings, enabled: true });
      fillWebAiSettingsForm();
      state.lastWebTalkAt = Date.now();
      setPetMood("study", message);
      showAiSettingsHint(`测试成功：${message}`, true);
    } catch (error) {
      showAiSettingsHint("测试失败。可能是 API key、模型名、endpoint 或浏览器跨域限制。");
      console.warn("Web pet AI test failed:", error);
    } finally {
      elements.aiTestBtn.disabled = false;
    }
  });

  elements.petAvatar.addEventListener("click", () => {
    reactToPetClick();
  });

  elements.dailyGoalInput.addEventListener("change", () => {
    const nextGoal = Math.max(5, Math.min(200, Number(elements.dailyGoalInput.value) || 30));
    state.dailyGoal = nextGoal;
    elements.dailyGoalInput.value = nextGoal;
    saveDailyGoal();
    renderStats();
  });

  elements.levelButtons.forEach((button) => {
    button.addEventListener("click", () => {
      elements.levelButtons.forEach((item) => item.classList.remove("active"));
      button.classList.add("active");
      state.level = button.dataset.level;
      state.reviewOnly = false;
      state.dueOnly = false;
      state.lookupOnly = false;
      renderAll();
      nextQuestion();
    });
  });

  elements.modeButtons.forEach((button) => {
    button.addEventListener("click", () => {
      elements.modeButtons.forEach((item) => item.classList.remove("active"));
      button.classList.add("active");
      state.mode = button.dataset.mode;
      state.reviewOnly = false;
      state.dueOnly = false;
      state.lookupOnly = false;
      nextQuestion();
    });
  });

  elements.reviewWrongBtn.addEventListener("click", () => {
    startPracticeMode("wrong");
  });

  elements.reviewDueBtn.addEventListener("click", () => {
    startPracticeMode("due");
  });

  const startLookupPractice = () => {
    startPracticeMode("lookup");
  };

  elements.reviewLookupBtn.addEventListener("click", startLookupPractice);
  elements.practiceLookupBtn.addEventListener("click", startLookupPractice);
  elements.planDueBtn.addEventListener("click", () => startPracticeMode("due"));
  elements.planLookupBtn.addEventListener("click", () => startPracticeMode("lookup"));
  elements.planWrongBtn.addEventListener("click", () => startPracticeMode("wrong"));

  elements.nextBtn.addEventListener("click", () => {
    nextQuestion();
  });

  elements.speakBtn.addEventListener("click", () => {
    speakWord();
  });

  elements.markReviewBtn.addEventListener("click", () => {
    markCurrentForReview();
  });

  elements.searchForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const query = cleanText(elements.searchInput.value);
    const target = normalize(query);
    if (!target) {
      renderSearchResults([], "");
      return;
    }
    const matches = allWords().filter((word) => {
      return wordKey(word).includes(target) || normalize(word.meaning).includes(target);
    });
    matches.slice(0, 5).forEach((word) => addToLookupPlan(word, "lookup"));
    if (matches.length > 0) {
      setPetMood("lookup", `已把 ${Math.min(matches.length, 5)} 个查词结果加入计划。`);
    }
    renderSearchResults(matches, query);
  });

  elements.addWordForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const spelling = normalize(elements.newWord.value);
    const meaning = cleanText(elements.newMeaning.value);
    const example = cleanText(elements.newExample.value);

    if (!spelling || !meaning) {
      flashSaveState("请填完整");
      return;
    }

    if (allWords().some((word) => wordKey(word) === spelling)) {
      flashSaveState("已存在");
      return;
    }

    state.customWords.push({
      id: `custom-${Date.now()}`,
      spelling,
      meaning,
      example,
      level: "自定义"
    });
    saveCustomWords();
    renderAll();
    elements.addWordForm.reset();
  });

  elements.clearCustomBtn.addEventListener("click", () => {
    if (state.customWords.length === 0) {
      flashSaveState("无自定义");
      return;
    }
    if (!window.confirm("确定清空当前账号的自定义词库吗？")) {
      return;
    }
    state.customWords = [];
    saveCustomWords();
    renderAll();
    nextQuestion();
  });
}

function init() {
  loadAccounts();
  loadWebAiSettings();
  const lastUser = localStorage.getItem(storageKey("last-user")) || "student";
  elements.loginUsername.value = lastUser;
  elements.authWordCount.textContent = `四六级 ${vocabMeta.totalCount || baseWords.length} 词`;
  showPage(pageFromHash());
  renderSearchResults([], "");
  bindEvents();
  bindFloatingPetDrag();
  elements.petAvatar.dataset.action = "idle";
  elements.floatingPet.dataset.action = "idle";
  window.setInterval(() => {
    if (state.username) {
      checkLoginPetBehaviors();
    }
  }, 60 * 1000);
  window.setInterval(playIdlePetAction, 14 * 1000);
  window.setInterval(petIdleTalk, 35 * 1000);
}

init();
